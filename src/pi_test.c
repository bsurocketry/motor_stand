
#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char ** argv) {

   gpioInitialise();
   getchar();

   printf("res0: %d\n",gpioInitialise());
   printf("res1: %d\n",gpioSetMode(23,PI_INPUT));
   printf("res2: %d\n",gpioSetMode(24,PI_OUTPUT));
   printf("res3: %d\n",gpioWrite(23,1));
   gpioDelay(100);

   while (1) {
      sleep(3);
      //gpioWrite(23,1);
      gpioDelay(100);
      printf("flipping %d\n",gpioRead(23));
      sleep(3);
      //gpioWrite(23,0);
      gpioDelay(100);
      printf("flipping %d\n",gpioRead(23));
   }

   return 0;
}
