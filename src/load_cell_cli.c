
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "load_cell.h"
#include "load_cell_cli.h"

#define SHINOBU_IS_BEST_GIRL 1
#define RUN_FAILURE ((pthread_t)-1)


struct load_cell_cli_thread_data {
   void (*callback)(output_data *, void *);
   void * uarg;
};

static void run_load_cell_cli_worker(void (*callback)(output_data *,
                                                      void * uarg),
                                     void * uarg) {

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
      return;
   }

   output_data message;
   
   while (SHINOBU_IS_BEST_GIRL) {

      recvfrom(sock,&message,sizeof(output_data),0,NULL,NULL);

      callback(&message,uarg);

   }

   return;
}

static void * load_cell_cli_thread_wrapper(void * arg) {
   struct load_cell_cli_thread_data * info =
                               (struct load_cell_cli_thread_data *)arg;

   run_load_cell_cli_worker(info->callback,info->uarg);
}

pthread_t run_load_cell_cli(void (* callback)(output_data *, void * uarg),
                            void * uarg, lc_cli_opt behavior) {

   switch (behavior) {
      case LC_CLI_DETACH: {

            pthread_t thread_id;
            struct load_cell_cli_thread_data thread_bootstrap_data;
            thread_bootstrap_data.callback = callback;
            thread_bootstrap_data.uarg     = uarg;

            if (pthread_create(&thread_id,NULL,load_cell_cli_thread_wrapper,
                               (void *)&thread_bootstrap_data)) {
               printf("failed to spawn new thread - internal error\n");
               return RUN_FAILURE;
            }

            return thread_id;
         }
         break;
      case LC_CLI_NODETACH:
         run_load_cell_cli_worker(callback,uarg);
         return RUN_FAILURE;
         break;
      default:
         printf("load_cell_cli: unknown behavior argument passed\n");
         return RUN_FAILURE;
         break;
   }
}
