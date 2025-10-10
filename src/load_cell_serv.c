
/* to build call:
 * gcc -pthread rocket_test_stand_data.c -lpigpio -lrt
 */

#include <signal.h>
#include "gpio_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "load_cell.h"

#define PIN_DT  23
#define PIN_SCK 24

#define GAIN_32  2
#define GAIN_64  3
#define GAIN_128 1

static int global = 1;

static void handler(int signum) {
   (void)signum;
   printf("finishing...\n");
   global = 0;
}

static void reset_adc() {
   gpioWrite(PIN_SCK,1);
   gpioDelay(100);
   gpioWrite(PIN_SCK,0);
   gpioDelay(5);
}

static int pop_bit() {
   /* toggle on, read, wait one microsecond, and toggle off */
   gpioWrite(PIN_SCK,1);
   gpioDelay(1);
   gpioWrite(PIN_SCK,0);
   int retval = gpioRead(PIN_DT);
   return retval;
}

static long next_value(int new_gain) {
   gpioWrite(PIN_SCK,0);
   while (gpioRead(PIN_DT)) {
      gpioDelay(1);
   }

   /* we have data to read! yay */
   uint value = 0;
   for (int i = 0; i < 24; ++i) {

      value <<= 1;
      value |= pop_bit();

   }


   for (int i = 0; i < new_gain; ++i)
      (void)pop_bit();

   //value ^= 0x800000;
   return value;
}


#define CC_IS_BEST_GIRL 1

static void * handle_auxillary_command(void * arg) {
   pthread_detach(pthread_self());
   struct auxillary_server_handler * handler = (struct auxillary_server_handler *)arg;

   struct auxillary_server_msg msg = {.value = 0, .type = -1};
   int res;

   do {
      res = read(handler->fd,&msg,sizeof(struct auxillary_server_msg));
   } while (res == -1 && errno == EINTR);

   switch (msg.type) {
      case OP_RESET: {

         int expected;
         do { 
            expected = atomic_load(handler->data.reset);
         } while (!atomic_compare_exchange_strong(handler->data.reset,&expected,1));

         }
         break;
      case OP_TARE:

         atomic_store(handler->data.tare,msg.value);

         break;
      case OP_ACK:
         break;

      default:
         /* do not respond at all */
         goto finish;
         break;
   }

   msg.type = OP_ACK;
   do {
      res = write(handler->fd,&msg,sizeof(struct auxillary_server_msg));
   } while (res == -1 && errno == EINTR);

finish:
   {
      close(handler->fd);
      free(handler);
      return NULL;
   }
}

static void * run_auxillary_server(void * arg) {
   pthread_detach(pthread_self());
   struct auxillary_server_data * data = (struct auxillary_server_data *)arg;

   int serv_port, cli_port;

   struct sockaddr_in servaddr, cliaddr;
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = AUX_PORT;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

   serv_port = socket(AF_INET, SOCK_STREAM, 0);
   if (serv_port == -1) {
      printf("Internal error: failed to instantiate serv port\n");
      return NULL;
   }

   if (bind(serv_port,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in)) == -1) {
      printf("Internal error: failed to bind auxillary server\n");
      return NULL;
   }

   if (listen(serv_port,5) == -1) {
      printf("Internal error: failed to call listen on auxillary server\n");
      return NULL;
   }

   printf("auxillary server started successfully\n");

   while (CC_IS_BEST_GIRL) {
      pthread_t tid;
      socklen_t socklen = sizeof(struct sockaddr_in);

      cli_port = accept(serv_port, (struct sockaddr *)&cliaddr, &socklen);

      struct auxillary_server_handler * handler = malloc(sizeof(struct auxillary_server_handler));
      memcpy(&handler->data,data,sizeof(struct auxillary_server_data));
      handler->fd = cli_port;

      pthread_create(&tid,NULL,handle_auxillary_command,handler);
   }


   return NULL;
}

int main(int argc, char ** argv) {

   if (argc != 4) {
help_message:
      printf("usage:\n%s [-c | -k] <known_weight/multiplier - float> <dest address>\n"
             "options:\n"
             "\t-c : calibrate the sensor, value passed is the known weight to use for calibration\n"
             "\t-k : the multiplier is known, the value passed is the multiplier for the sensor\n"
             "arguments:\n"
             "\tknown_weight/multiplier : a list of values. If the c option is pased these values will be used\n"
             "                            to perform a first order least squares calibration. The values should\n"
             "                            be passed as a comma seperated list in quotes, i.e. \"1,2,3,4,5\"\n"
             "                            If the k option is passed these values will be used as the first and\n"
             "                            zeroth term in the least squares fit, i.e. in the equation mx + b the\n"
             "                            values should be passed in the form \"m,b\".\n"
             "\tdest address            : the address to send UDP data packets to. The output socket is set\n"
             "\t                          to enable broadcast, so you may pass a broadcast address as well\n",
             argv[0]);
      exit(1);
   }

   /* signal handler */
   struct sigaction action;
   bzero(&action,sizeof(struct sigaction));
   action.sa_handler = handler;

   /* server setup */
   int sock = socket(AF_INET,SOCK_DGRAM,0);
   if (sock == -1) {
      perror("failed to initalize server socket");
      exit(1);
   }

   {
      int yes = 1;
      if (setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(int)) == -1) {
         perror("failed to set socket to broadcast");
         exit(1);
      }
   }

   double * known_weights, multiplier_b, multiplier_m;
   _Atomic(double) tare;
   atomic_init(&tare,0.0);
   atomic_int reset;
   atomic_init(&reset,0);
   int calibrating = 0, value_count;

   if (strcmp("-c",argv[1]) == 0) {
      calibrating = 1;

      char * pos = argv[2];
      int value_count = 0;
      known_weights = malloc(sizeof(double) * 2);

      do {

         /* simply double the array at every power of two, do
          * not keep track of the current size of the array.
          * This works because we are only growing, we don't
          * need to worry about seeing the same power of two
          * twice.                                           */

         if (__builtin_popcount(value_count) == 1)
            known_weights = realloc(known_weights,value_count * 2);

         known_weights[value_count] = strtod(pos + 1,&pos);
         value_count += 1;

      } while (pos[0] == ',');

      printf("got known weights as");
      for (int i = 0; i < value_count; ++i)
         printf(" %lf",known_weights[i]);
      printf("\n");
   }
   else if (strcmp("-k",argv[1]) == 0) {
      multiplier_m = strtod(argv[2],&argv[2]);
      if (argv[2][0] != ',') {
         printf("Expected values in form \"m,b\" for option -k\n");
         exit(1);
      }
      multiplier_b = strtod(argv[2] + 1,NULL);
      printf("got multiplier as %lf * x + %lf\n",multiplier_m,multiplier_b);
   }
   else if (strcmp("-h",argv[1]) == 0) {
      goto help_message;
   }
   else {
      printf("unknown argument given: %s\nplease provide one of: [ -c | -k | -h ]\n",argv[1]);
      exit(1);
   }

   struct sockaddr_in dest_addr;
   bzero(&dest_addr,sizeof(struct sockaddr_in));
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons(IN_PORT);
   if (inet_pton(AF_INET,argv[3],&dest_addr.sin_addr.s_addr) != 1) {
      printf("failed to convert given address to an IPv4 addr\n");
      perror("error");
      exit(1);
   }

   printf("got dest address as %s\n",argv[3]);

   /* initialize the library */
   if (gpioInitialise() < 0) {
      printf("failed to initialize pigpio\n");
      exit(1);
   }

   /* initialize the gpio pins */
   if (gpioSetMode(PIN_DT, PI_INPUT) < 0) {
      printf("failed to initialize PIN_DT\n");
      exit(1);
   }
   if (gpioSetMode(PIN_SCK, PI_OUTPUT) < 0) {
      printf("failed to initialize PIN_SCK\n");
      exit(1);
   }

   if (calibrating) {

      double * timestep = malloc(sizeof(double)*value_count);
      double * measurement = malloc(sizeof(double)*value_count);

      for (int i = 0; i < value_count; ++i) {
   
         printf("place known weight %lf on scale and press any key\n",known_weights[i]);
         (void)getc(stdin);

         reset_adc();
         long value_raw = next_value(GAIN_128);

         // 0xffffff is max buffer value
         // double value = (double)((double)value_raw / (double)0xffffff);

         timestep[i] = (double)value_raw;
         measurement[i] = known_weights[i];

      }

      first_order_least_squares_filter(timestep,measurement,value_count,
                                       &multiplier_m,&multiplier_b);

      printf("got multiplier as %lf * x + %lf\n--> m = %lf\n--> b = %lf",
             multiplier_m,multiplier_b,mutliplier_m,multiplier_b);
      printf("you can save these value for reuse without calibrating via the -k option and by passing them as \"m,b\"\n");


   }

   sigaction(SIGINT,&action,NULL);

   printf("launching auxillary server\n");
   pthread_t tid;
   struct auxillary_server_data data = { .tare = &tare, .reset = &reset };
   pthread_create(&tid,NULL,run_auxillary_server,&data);

   printf("starting\n");
   reset_adc();
   printf("sending data...\n");

   struct timespec time;

   long prev_raw = 0;
   /* main program loop */
   while (global) {

      long next_raw = next_value(GAIN_128);
      double next = (double)((double)next_raw / (double)0xffffff) * multiplier_m + multiplier_b;
      clock_gettime(CLOCK_MONOTONIC,&time);
      //clock_gettime(CLOCK_MONOTONIC_COARSE,&time);
      long microseconds = (time.tv_sec * 1000000) + (time.tv_nsec / 1000);

      output_data o = {.collect_time_sec = time.tv_sec,
                       .collect_time_nsec = time.tv_nsec,
                       .collect_time = microseconds,
                       .raw_value = next_raw, 
                       .value = next,
                       .value_tare = next - atomic_load(&tare)};

      sendto(sock,&o,sizeof(output_data),0,
             (struct sockaddr *)&dest_addr,sizeof(struct sockaddr_in));

      if (atomic_load(&reset)) {

         int expected;
         do { 
            expected = atomic_load(&reset);
         } while (!atomic_compare_exchange_strong(&reset,&expected,0));

         reset_adc();
      }
   }

   printf("finishing\n");

   /* close the library connection */
   gpioTerminate();
   close(sock);

   return 0;
}
