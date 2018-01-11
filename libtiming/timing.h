/*
 * timing.h
 * header file for time related functioncs
 */

extern void *timing_new_timer(int nanoprecision);
void timing_delete_timer(void *t);
void timing_start(void *t);
double timing_end(void *t);
