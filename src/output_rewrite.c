
/* takes an output file and rewrites the timesteps to start at zero. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "load_cell.h"

int main(int argc, char ** argv) {

   if (argc != 2) {
      printf("usage:\n%s <log file>\n",argv[0]);
      exit(1);
   }


   FILE * fptr = fopen(argv[1],"r");

   if (fptr == NULL) {
      printf("unable to open file %s\n",argv[1]);
      perror("fopen");
      exit(1);
   }

   char buf[1024];
   int epos;

   output_data * data_vec = malloc(sizeof(output_data)*256);
   int data_vec_pos = 0, data_vec_len = 256;

   while (fgets(buf,sizeof(buf),fptr)) {

      /* skip any lines where a digit is not the first character */
      if (!isdigit(buf[0])) {
         buf[strlen(buf)-1] = '\0';
         printf("skipping line: \"%s\"\n",buf);
         continue;
      }

      char * pos = buf;
      long collect_time = strtol(pos,&pos,0);
      long raw = strtol(pos,&pos,0);
      double value = strtod(pos,&pos);
      double value_tare = strtod(pos,&pos);

      if (data_vec_pos == data_vec_len) {
         data_vec_len *= 2;
         data_vec = realloc(data_vec,sizeof(output_data)*data_vec_len);
      }

      data_vec[data_vec_pos].collect_time = collect_time;
      data_vec[data_vec_pos].raw_value    = raw;
      data_vec[data_vec_pos].value        = value;
      data_vec[data_vec_pos].value_tare   = value_tare;

      data_vec_pos += 1;
   }

   if (!data_vec_pos)
      exit(0);

   fclose(fptr);

   strcpy(buf,argv[1]);
   strcat(buf,".adjust.log");

   FILE * outfptr = fopen(buf,"a");

   time_t collect_offset = data_vec[0].collect_time;
   fprintf(outfptr,"collect_time raw_value value tare_value\n");
   for (int i = 0; i < data_vec_pos; ++i) {
      fprintf(outfptr,"%ld\t%ld\t%lf\t%lf\n",
              data_vec[i].collect_time - collect_offset,
              data_vec[i].raw_value,
              data_vec[i].value,
              data_vec[i].value_tare);
   }

   fclose(outfptr);

   return 0;
}
