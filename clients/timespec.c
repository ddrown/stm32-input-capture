#include <time.h>
#include <stdio.h>
#include <stdint.h>

#include "timespec.h"

void add_timespecs(struct timespec *result, const struct timespec *add) {
  /* added_offset_ns = -1000
   * previous tv_sec =  0, tv_nsec = -999999000
   *      new tv_sec = -1, tv_nsec = 0
   *
   * added_offset_ns = -2000
   * previous tv_sec =  0, tv_nsec = -999999000
   *      new tv_sec = -1, tv_nsec = -1000
   *
   * added_offset_ns = 1000
   * previous tv_sec =  0, tv_nsec = 999999000
   *      new tv_sec =  1, tv_nsec = 0
   *
   * added_offset_ns = 2000
   * previous tv_sec =  0, tv_nsec = 999999000
   *      new tv_sec =  1, tv_nsec = 1000
   *
   * added_offset_ns = 2000
   * previous tv_sec = -2, tv_nsec = -1000
   *      new tv_sec = -1, tv_nsec = -999999000
   */

  result->tv_nsec += add->tv_nsec;
  result->tv_sec += add->tv_sec;

  if(result->tv_nsec >= 1000000000) {
    result->tv_sec++;
    result->tv_nsec -= 1000000000;
  } else if(result->tv_nsec <= -1000000000) {
    result->tv_sec--;
    result->tv_nsec += 1000000000;
  } 

  if(result->tv_sec < 0 && result->tv_nsec > 0) {
    result->tv_sec++;
    result->tv_nsec -= 1000000000;
  } else if(result->tv_sec > 0 && result->tv_nsec < 0) {
    result->tv_sec--;
    result->tv_nsec += 1000000000;
  }
}

void sub_timespecs(struct timespec *result, struct timespec *sub) {
  sub->tv_sec *= -1;
  sub->tv_nsec *= -1;
  add_timespecs(result, sub);
  sub->tv_sec *= -1;
  sub->tv_nsec *= -1;
}

void sub_timespecs3(struct timespec *result, const struct timespec *o1, struct timespec *o2) {
  result->tv_sec = o1->tv_sec;
  result->tv_nsec = o1->tv_nsec;
  sub_timespecs(result, o2);
}

void print_timespec(struct timespec *t) {
  if(t->tv_sec < 0 || t->tv_nsec < 0) {
    t->tv_sec *= -1;
    t->tv_nsec *= -1;
    printf("-%ld.%09ld", t->tv_sec, t->tv_nsec);
    t->tv_sec *= -1;
    t->tv_nsec *= -1;
  } else {
    printf("%ld.%09ld", t->tv_sec, t->tv_nsec);
  }
}

double timespec_to_double(const struct timespec *ts) {
  double retval;
  
  retval = ts->tv_sec;
  retval += ts->tv_nsec / 1000000000.0;

  return retval;
}

void double_to_timespec(struct timespec *ts, double time) {
  ts->tv_sec = time / 1000000000.0;
  ts->tv_nsec = (uint64_t)(time) % 1000000000;
}
