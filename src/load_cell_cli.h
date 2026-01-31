
/* interface for running a client of the load cell server. 
   This should be a pretty minimal interface that allows for
   easy extension and use of the server data.                */

#ifndef LOAD_CELL_CLI

#define LOAD_CELL_CLI

#include <pthread.h>

#include "load_cell.h"

typedef enum {
   LC_CLI_DETACH,
   LC_CLI_NODETACH,
} lc_cli_opt;

/* function performs as follows:
      A socket is setup to connect to the load cell server that is 
      percieved as running. After this, whenever data comes through
      this socket, the callback function is invoked and passed the
      output_data struct and the given uarg. 

   args:
      callback   - function to run on data collection
      uarg       - user provided argument to the callback
      lc_cli_opt - behavior of this call. Differs as follows:
                   LC_CLI_DETACH   - spawn a new thread to run the callback
                                     return the thread's pid
                   LC_CLI_NODETACH - do not spawn a new thread. Don't return
   returns:
      -1  : an error occured, is printed to terminal 
      pid : LC_CLI_DETACH called, no issues occurred
*/

#if FAST_AS_POSSIBLE
typedef void (*callback_func)(output_batch *, void *);
#else
typedef void (*callback_func)(output_data *, void *);
#endif


pthread_t run_load_cell_cli(callback_func callback,
                            void * uarg, lc_cli_opt behavior);



#endif
