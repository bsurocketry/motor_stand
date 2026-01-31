
/* program that reads in server output and dumps it to a logfile.
   If "-" is passed as the file name logs will be dumped to stdout 

   Note: this is a appending write, so it will add to an existing file
*/

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "load_cell.h"
#include "load_cell_cli.h"
#include "adc.h"

static FILE * fhandle = NULL;

static void handler(int signum) {
   printf("closing file...\n");
   fclose(fhandle);
   fsync(fileno(fhandle));
   printf("exiting...\n");
   _exit(0);
}

static void write_callback(output_data * data, void * uarg) {
   FILE * fhandle = (FILE *)uarg;
   char buf[128];
   snprintf(buf,128,"%ld\t%ld\t%lf\t%lf\n",data->minimal.collect_time,
            data->minimal.raw_value, data->value, data->value_tare);
   fwrite(buf,strlen(buf),sizeof(char),fhandle);
}

static void write_callback_batch(output_batch * batch, void * uarg) {
   logger_data * l_data = (logger_data *)uarg;
   FILE * fhandle = l_data->fhandle;
   double m = l_data->m;
   double b = l_data->b;
   char buf[128];
   for (int i = 0; i < batch->count; ++i) {
      output_minimal * min = &batch->data[i];
      double value = m * ((double)min->raw_value / (double)ADC_MAX) + b;
      snprintf(buf,128,"%ld\t%ld\t%lf\t%lf\n",min->collect_time,
               min->raw_value, value, 0.0);
      fwrite(buf,strlen(buf),sizeof(char),fhandle);
   }
}

int main(int argc, char ** argv) {

   if ((argc != 2 && argc != 4) || (argc >= 2 && strcmp("-h",argv[1]) == 0)) {
help_msg:
      printf("usage:\n%s <fname> { -k <m,b> }\n"
             "passing '-' as fname will print to stdout\n"
             "passing -k followed by the conversion constants\n"
             "will allow batch data to be reconstructed live\n",
             argv[0]);
      exit(1);
   }

   struct sigaction action;
   bzero(&action,sizeof(struct sigaction));
   action.sa_handler = handler;

   sigaction(SIGINT,&action,NULL);

   if (strcmp("-",argv[1]) == 0)
      fhandle = stdout;
   else
      fhandle = fopen(argv[1],"a");

#if FAST_AS_POSSIBLE
   double m = 0.0;
   double b = 0.0;

   if (argc == 4) {
      if (strcmp("-k",argv[2]))
         goto help_msg;
      m = strtod(argv[3],&argv[2]);
      if (argv[3][0] != ',') {
         printf("Expected values in form \"m,b\" for option -k\n");
         exit(1);
      }
      b = strtod(argv[3] + 1,NULL);
      printf("got multiplier as %lf * x + %lf\n",m,b);
   }

   logger_data l_data = {
      .fhandle = fhandle,
      .m = m, .b = b,
   };
#endif

   if (!fhandle) {
      printf("failed to open given file: %s\n",argv[2]);
      exit(1);
   }

   /* write out a comment - not sure if this will break csv readers */
   char buf[128];
   snprintf(buf,128,"# data collected %s %s\n",__DATE__,__TIME__);
   fwrite(buf,strlen(buf),sizeof(char),fhandle);

   /* write out the header for the data */
   snprintf(buf,128,"collect_time\traw_value\tvalue\ttare_value\n");
   fwrite(buf,strlen(buf),sizeof(char),fhandle);
   
   /* launch the cli handle */
#if FAST_AS_POSSIBLE
   run_load_cell_cli(write_callback_batch,&l_data,LC_CLI_NODETACH);
#else
   run_load_cell_cli(write_callback,fhandle,LC_CLI_NODETACH);
#endif

   return 0;
}
