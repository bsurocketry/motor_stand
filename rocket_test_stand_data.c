
/* to build call:
 * gcc -pthread rocket_test_stand_data.c -lpigpio -lrt
 */

#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PIN_DT  23
#define PIN_SCK 4

#define GAIN_32  2
#define GAIN_64  3
#define GAIN_128 1

#define IN_PORT 8777

typedef struct {
   time_t collect_time;
   long raw_value;
   double value;
} output_data;

int global = 1;

void handler(int sig) {
   (void)sig;
   global = 0;
   return;
}

static int pop_bit() {
   /* toggle on, read, wait one microsecond, and toggle off */
   gpioWrite(PIN_SCK,1);
   gpioDelay(1);
   gpioWrite(PIN_SCK,0);
   return gpioRead(PIN_DT);
}

static long next_value(int new_gain) {
   while (gpioRead(PIN_DT)) {
      printf("waiting\n");
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

   value ^= 0x800000;
   return value;
}

int main(int argc, char ** argv) {

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

   if (argc != 3) {
      printf("usage:\n%s <known_weight - float> <dest address>\n",argv[0]);
      exit(1);
   }

   double known_weight = strtod(argv[1],NULL);
   printf("got known weight as %lf\n",known_weight);

   struct sockaddr_in dest_addr;
   bzero(&dest_addr,sizeof(struct sockaddr_in));
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons(IN_PORT);
   if (inet_pton(AF_INET,argv[2],&dest_addr.sin_addr.s_addr) != 1) {
      printf("failed to convert given address to an IPv4 addr\n");
      perror("error output");
      exit(1);
   }

   printf("got dest address as %s\n",argv[2]);

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

   printf("write result: %d\n",gpioWrite(PIN_SCK,0));

   printf("place known weight on scale and press any key\n");
   (void)getc(stdin);

   long value_raw = next_value(GAIN_128);

   // 0xffffff is max buffer value
   double value = (double)((double)value_raw / (double)0xffffff);
   double multiplier = known_weight / value;

   printf("got multiplier as %lf\n",multiplier);

   printf("looping *cope*\n");

   sleep(5);

   /* main program loop */
   while (global) {

      long next_raw = next_value(GAIN_128);
      double next = (double)((double)next_raw / (double)0xffffff) * multiplier;

      output_data o = {.collect_time = time(NULL), .raw_value = next_raw, .value = next};

      sendto(sock,&o,sizeof(output_data),0,
             (struct sockaddr *)&dest_addr,sizeof(struct sockaddr_in));

   }

   printf("finishing\n");

   /* close the library connection */
   gpioTerminate();

   return 0;
}
