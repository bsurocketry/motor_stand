
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
   snprintf(buf,128,"%ld %ld %lf\n",data->collect_time,
            data->raw_value, data->value);
   fwrite(buf,strlen(buf),sizeof(char),fhandle);
}

int main(int argc, char ** argv) {

   if (argc != 2 || (argc >= 2 && strcmp("-h",argc[1]) == 0)) {
      printf("usage:\n%s <fname>\n"
             "passing '-' as fname will print to stdout\n",
             argv[0]);
      exit(1);
   }

   struct sigaction action;
   bzero(&action,sizeof(struct sigaction));
   action.sa_handler = handler;

   sigaction(SIGINT,&action,NULL);

   if (strcmp("-",argv[2]) == 0)
      fhandle = stdout;
   else
      fhandle = fopen(argv[2],"a");

   if (!fhandle) {
      printf("failed to open given file: %s\n",argv[2]);
      exit(1);
   }

   /* write out a comment - not sure if this will break csv readers */
   char buf[128];
   snprintf(buf,128,"# data collected %s %s\n",__DATE__,__TIME__);
   fwrite(buf,strlen(buf),sizeof(char),fhandle);

   /* write out the header for the data */
   snprintf(buf,128,"collect_time, raw_value, value\n");
   fwrite(buf,strlen(buf),sizeof(char),fhandle);
   
   /* launch the cli handle */
   run_load_cell_cli(write_callback,fhandle,LC_CLI_NODETACH);

   return 0;
}
