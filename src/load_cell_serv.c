
/* to build call:
 * gcc -pthread rocket_test_stand_data.c -lpigpio -lrt
 */

#include <signal.h>
#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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
   return gpioRead(PIN_DT);
}

static long next_value(int new_gain) {
   while (gpioRead(PIN_DT)) {
      //printf("waiting\n");
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

   if (argc != 4) {
help_message:
      printf("usage:\n%s [-c | -k] <known_weight/multiplier - float> <dest address>\n"
             "options:\n"
             "\t-c : calibrate the sensor, value passed is the known weight to use for calibration\n"
             "\t-k : the multiplier is known, the value passed is the multiplier for the sensor\n"
             "arguments:\n"
             "\tknown_weight/multiplier : the value to either use as the known weight to calibrate from or\n"
             "\t                          as the multiplier generated from a previous calibration.\n"
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

   double known_weight, multiplier;
   int calibrating = 0;

   if (strcmp("-c",argv[1]) == 0) {
      calibrating = 1;
      known_weight = strtod(argv[2],NULL);
      printf("got known weight as %lf\n",known_weight);
   }
   else if (strcmp("-k",argv[1]) == 0) {
      multiplier = strtod(argv[2],NULL);
      printf("got multiplier as %lf\n",multiplier);
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

   if (calibrating) {
   
      printf("place known weight on scale and press any key\n");
      (void)getc(stdin);

      reset_adc();
      long value_raw = next_value(GAIN_128);

      // 0xffffff is max buffer value
      double value = (double)((double)value_raw / (double)0xffffff);
      multiplier = known_weight / value;

      printf("got multiplier as %lf\n",multiplier);
      printf("you can save this value for reuse without calibrating via the -k option\n");
   }

   sigaction(SIGINT,&action,NULL);

   printf("starting\n");
   reset_adc();
   printf("sending data...\n");

   struct timespec time;

   /* main program loop */
   while (global) {

      long next_raw = next_value(GAIN_128);
      double next = (double)((double)next_raw / (double)0xffffff) * multiplier;
      clock_gettime(CLOCK_MONOTONIC,&time);
      //clock_gettime(CLOCK_MONOTONIC_COARSE,&time);
      long microseconds = (time.tv_sec * 1000000) + (time.tv_nsec / 1000);

      output_data o = {.collect_time_sec = time.tv_sec,
                       .collect_time_nsec = time.tv_nsec,
                       .collect_time = microseconds,
                       .raw_value = next_raw, .value = next};


      sendto(sock,&o,sizeof(output_data),0,
             (struct sockaddr *)&dest_addr,sizeof(struct sockaddr_in));
   }

   printf("finishing\n");

   /* close the library connection */
   gpioTerminate();
   close(sock);

   return 0;
}
