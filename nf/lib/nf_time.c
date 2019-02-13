#include "nf_time.h"

#undef time_t // catch potential mismatch
#include <time.h>

time_t current_time(void)
//@ requires last_time(?x);
//@ ensures result >= 0 &*& x <= result &*& last_time(result);
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);

#ifdef NSEC_TIMESTAMPS
  return (tp.tv_sec*1e9 + tp.tv_nsec);
#endif 

  return tp.tv_sec;
}
