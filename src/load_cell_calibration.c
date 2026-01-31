
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

#include "numerical_basics.h"
#include "method_of_least_squares.h"
#include "load_cell.h"
#include "adc.h"

#define SHINOBU_IS_BEST_GIRL 1
#define BUFLEN 129

int main(void) {

   int buf_len = 4;
   int buf_pos = 0;

   double * measurement = malloc(sizeof(double)*buf_len);
   double * known_values = malloc(sizeof(double)*buf_len);

   init_adc();

   printf("motor stand calibration\n");
   printf("enter q to finish calibration and return result\n");

   while (SHINOBU_IS_BEST_GIRL) {
      char buf[BUFLEN];
      fprintf(stderr,"> ");
      fgets(buf,BUFLEN-1,stdin);
      buf[strlen(buf)-1] = '\0';

      if (buf[0] == '\0')
         continue;
      if (strcmp(buf,"b") == 0) {
         if (buf_pos != 0) {
            buf_pos -= 1;
            printf("undoing estimate %lf, measurement %lf\n",
                    known_values[buf_pos],measurement[buf_pos]);
         }
         else {
            printf("already at start of buffer!\n");
         }
      }
      else if (strcmp(buf,"q") == 0) {
         printf("finishing...\n");
         break;
      }
      else {

         if (buf_pos == buf_len) {
            measurement = realloc(measurement,buf_len*2);
            known_values = realloc(known_values,buf_len*2);
            buf_len *= 2;
         }

         known_values[buf_pos] = strtold(buf,NULL);
         printf("got known value as %lf\n",known_values[buf_pos]);
         printf("place weight on scale and press enter...");
         printf("[ENTER]");

         (void)getc(stdin);

         reset_adc();
         long value_raw = next_value(GAIN_128);

         measurement[buf_pos] = (((double)value_raw) / (double)ADC_MAX);
         printf("got measurement as %lf\n",measurement[buf_pos]);

         buf_pos += 1;
      }
   }

   if (buf_pos == 0) {
      printf("no values collected, exiting\n");
   }
   else {
      printf("computing calibration constants with %d values\n",buf_pos);

      double m;
      double b;

      double m_2;
      double b_2;

      first_order_least_squares_filter(measurement,known_values,
                                       buf_pos,&b,&m);
      /*
      stable_first_order_least_squares(measurement,known_values,
                                       buf_pos,&m_2,&b_2);
      */

      printf("got multiplier as %lf * x + %lf\n-->m = %lf\n--> b = %lf\n",
             m,b,m,b);
      /*
      printf("got multiplier as %lf * x + %lf\n-->m = %lf\n--> b = %lf\n",
             m_2,b_2,m_2,b_2);
      */

      printf("known values and computed estimates:\n");
      for (int i = 0; i < buf_pos; ++i) {
         printf("K: %lf\t\tM: %lf\n",known_values[i],
                measurement[i]*m + b);
      }
      /*
      printf("known values and computed estimates:\n");
      for (int i = 0; i < buf_pos; ++i) {
         printf("K: %lf\t\tM: %lf\n",known_values[i],
                measurement[i]*m_2 + b_2);
      }
      */
   }

   close_adc();
   gpioTerminate();

   printf("finished\n");

   return 0;
}
