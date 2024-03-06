/*
 * timing.h
 * header file for time related functioncs
 */

#ifndef TIMING_H
#define TIMING_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIMING_NANOPRECISION 1

extern void *timing_new_timer(int nanoprecision);
void timing_delete_timer(void *t);
void timing_start(void *t);
double timing_end(void *t);

#ifdef __cplusplus
}
#endif

#endif