
#ifndef LOAD_CELL

#define LOAD_CELL
#include <time.h>
#include <stdio.h>

#define FAST_AS_POSSIBLE 1
#define BATCH_SIZE 430

typedef struct {
   time_t collect_time;
   long raw_value;
} output_minimal;

typedef struct {
   output_minimal minimal;
   time_t collect_time_sec;
   long collect_time_nsec;
   double value;
   double value_tare;
} output_data;

#ifdef FAST_AS_POSSIBLE
typedef struct {
   size_t count;
   output_minimal data[BATCH_SIZE];
} output_batch;

typedef struct {
   FILE * fhandle;
   double m;
   double b;
} logger_data;
#endif

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
