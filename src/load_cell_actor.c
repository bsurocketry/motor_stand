
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "load_cell.h"
#include "load_cell_cli.h"

struct average_data {
   double value;
   double value_tare;
   double sum;
   double sum_tare;
   int count;
   pthread_mutex_t mux;
};

static void write_callback(output_data * data, void * uarg) {
   struct average_data * avg = (struct average_data *)uarg;
   pthread_mutex_lock(&avg->mux);

   avg->sum += data->value;
   avg->sum_tare += data->value_tare;
   avg->count += 1;
   avg->value = avg->sum / avg->count;
   avg->value_tare = avg->sum_tare / avg->count;

   pthread_mutex_unlock(&avg->mux);
}

static void get_server_address(struct sockaddr_in * servaddr) {

   int sock = socket(AF_INET,SOCK_DGRAM,0);
   if (sock == -1) {
      printf("failed to instantiate socket - internal error\n");
      return;
   }

   int yes = 1;
   if (setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&yes,sizeof(int)) == -1) {
      printf("failed to set reuseport on socket - internal error\n");
      return;
   }

   struct sockaddr_in cli_addr;
   bzero(&cli_addr,sizeof(struct sockaddr_in));
   cli_addr.sin_family = AF_INET;
   cli_addr.sin_port = htons(IN_PORT);
   cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(sock,(struct sockaddr *)&cli_addr,
               sizeof(struct sockaddr_in)) == -1) {
      printf("failed to bind socket to address - internal error\n");
      perror("bind");
      return;
   }

   output_data message;
   
   socklen_t len = sizeof(struct sockaddr_in);
   recvfrom(sock,&message,sizeof(output_data),0,(struct sockaddr *)servaddr,&len);
   close(sock);

   servaddr->sin_port = AUX_PORT;
}

static int send_server_message(auxillary_op_type op, double value) {
   struct sockaddr_in out_addr;
   get_server_address(&out_addr);

   struct auxillary_server_msg msg;
   bzero(&msg,sizeof(struct auxillary_server_msg));
   msg.value = value;
   msg.type = op;

   int sock = socket(AF_INET,SOCK_STREAM,0), res;
   if (sock == -1) {
      printf("failed to create socket - internal error\n");
      return 0;
   }

   struct sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = 0;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(sock,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)) == -1) {
      printf("failed to bind socket to address - internal error\n");
      close(sock);
      perror("bind");
      return 0;
   }

   if (connect(sock,(struct sockaddr *)&out_addr,sizeof(struct sockaddr_in)) == -1) {
      printf("failed to connect socket to remote host - internal error\n");
      perror("connect");
      close(sock);
      return 0;
   }

   /* now we can finally send the message! */
   do {
      res = write(sock,&msg,sizeof(msg));
   } while (res == -1 && errno == EINTR);

   if (res == -1) {
      printf("failed to write to remote server - internal error\n");
      close(sock);
      return 0;
   }

   do {
      res = read(sock,&msg,sizeof(msg));
   } while (res == -1 && errno == EINTR);

   if (res == -1) {
      printf("failed to read from remote server - internal error\n");
      close(sock);
      return 0;
   }

   if (msg.type != OP_ACK) {
      close(sock);
      return 0;
   }

   return 1;
}

static void reset_average(struct average_data * avg) {
   pthread_mutex_lock(&avg->mux);

   avg->sum = 0;
   avg->sum_tare = 0;
   avg->count = 0;
   avg->value = 0;
   avg->value_tare = 0;

   pthread_mutex_unlock(&avg->mux);
}

int main(int argc, char ** argv) {

   if (argc != 2) {
      printf("usage:\n%s [ ping | reset | tare ]\n",argv[0]);
      exit(1);
   }

   struct average_data avg = { .mux = PTHREAD_MUTEX_INITIALIZER };
   reset_average(&avg);

   run_load_cell_cli(write_callback,(void *)&avg,LC_CLI_DETACH);

   if (strcmp(argv[1], "ping") == 0) {
      printf("ping\n");
      if (send_server_message(OP_ACK,0.0))
         printf("pong\n");
      else printf("ping failed\n");
   }
   else if (strcmp(argv[1], "reset") == 0) {
      printf("resetting...\n");
      if (send_server_message(OP_RESET,0.0))
         printf("reset succeeded\n");
      else printf("reset failed\n");
   }
   else if (strcmp(argv[1], "tare") == 0) {
      printf("collecting prior average\n");
      reset_average(&avg);
      sleep(5);

      pthread_mutex_lock(&avg.mux);
      double prior = avg.value;
      pthread_mutex_unlock(&avg.mux);
      printf("prior average: %lf\n",prior);

      printf("performing tare\n");
      if (send_server_message(OP_TARE,prior)) {
         printf("tare succeeded\n");
         printf("collecting post average\n");
         reset_average(&avg);
         sleep(5);

         pthread_mutex_lock(&avg.mux);
         double post = avg.value;
         pthread_mutex_unlock(&avg.mux);
         printf("prior average: %lf\n",post);
      }
      else printf("tare failed\n");
   }

   return 0;
}
