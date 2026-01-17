
#include <stdlib.h>
#include <stdio.h>
#include <pigpio.h>

void init_adc() {
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
}

void reset_adc() {
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

long next_value(int new_gain) {
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

void close_adc() {
   /* nop */
}
