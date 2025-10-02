
#ifndef LOAD_CELL

#define LOAD_CELL
#include <time.h>

typedef struct {
   time_t collect_time_sec;
   long collect_time_nsec;
   time_t collect_time;
   long raw_value;
   double value;
   double value_tare;
} output_data;

struct auxillary_server_data {
   _Atomic(double) * tare;
   _Atomic(int) * reset;
};

struct auxillary_server_handler {
   struct auxillary_server_data data;
   int fd;
};

typedef enum {
   OP_RESET,
   OP_TARE,
   OP_ACK,
} auxillary_op_type;

struct auxillary_server_msg {
   double value;
   auxillary_op_type type;
};

#define IN_PORT 8777
#define AUX_PORT 8778
#define MAX_CONNECTIONS 128

#endif
