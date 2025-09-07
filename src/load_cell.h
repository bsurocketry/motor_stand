
#ifndef LOAD_CELL

#define LOAD_CELL

typedef struct {
   time_t collect_time_sec;
   long collect_time_nsec;
   time_t collect_time;
   long raw_value;
   double value;
} output_data;

#define IN_PORT 8777

#endif
