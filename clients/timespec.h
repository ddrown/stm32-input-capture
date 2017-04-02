#ifndef TIMESPEC_H
#define TIMESPEC_H

void add_timespecs(struct timespec *result, const struct timespec *add);
void sub_timespecs(struct timespec *result, struct timespec *sub);
void sub_timespecs3(struct timespec *result, const struct timespec *o1, struct timespec *o2);
void print_timespec(struct timespec *t);
void double_to_timespec(struct timespec *ts, double time);
double timespec_to_double(const struct timespec *ts);

#endif
