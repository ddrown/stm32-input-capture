#ifndef TIMESPEC_H
#define TIMESPEC_H

void add_timespecs(struct timespec *result, struct timespec *add);
void sub_timespecs(struct timespec *result, struct timespec *sub);
void print_timespec(struct timespec *t);

#endif
