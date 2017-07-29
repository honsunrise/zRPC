//
// Created by zhsyourai on 12/26/16.
//

#ifndef ZRPC_TIME_H
#define ZRPC_TIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* The clocks we support. */
typedef enum {
    /* Monotonic clock. Epoch undefined. Always moves forwards. */
            zRPC_CLOCK_MONOTONIC = 0,
    /* Realtime clock. May jump forwards or backwards. Settable by
       the system administrator. Has its epoch at 0:00:00 UTC 1 Jan 1970. */
            zRPC_CLOCK_REALTIME,
    /* CPU cycle time obtained by rdtsc instruction on x86 platforms. Epoch
       undefined. Degrades to zRPC_CLOCK_REALTIME on other platforms. */
            zRPC_CLOCK_PRECISE,
    /* Unmeasurable clock type: no base, created by taking the difference
       between two times */
            zRPC_TIMESPAN
} zRPC_clock_type;

/* Analogous to struct timespec. On some machines, absolute times may be in
 * local time. */
typedef struct zRPC_timespec {
    int64_t tv_sec;
    int32_t tv_nsec;
    /** Against which clock was this time measured? (or zRPC_TIMESPAN if
        this is a relative time meaure) */
    zRPC_clock_type clock_type;
} zRPC_timespec;

/* Time constants. */
zRPC_timespec zRPC_time_0(zRPC_clock_type type); /* The zero time interval. */
zRPC_timespec zRPC_time_inf_future(zRPC_clock_type type); /* The far future */
zRPC_timespec zRPC_time_inf_past(zRPC_clock_type type);   /* The far past. */

#define zRPC_MS_PER_SEC 1000
#define zRPC_US_PER_SEC 1000000
#define zRPC_NS_PER_SEC 1000000000
#define zRPC_NS_PER_MS 1000000
#define zRPC_NS_PER_US 1000
#define zRPC_US_PER_MS 1000

/* Return the current time measured from the given clocks epoch. */
zRPC_timespec zRPC_now(zRPC_clock_type clock);

/* Convert a timespec from one clock to another */
zRPC_timespec zRPC_convert_clock_type(zRPC_timespec t,
                                      zRPC_clock_type target_clock);

/* Return -ve, 0, or +ve according to whether a < b, a == b, or a > b
   respectively.  */
int zRPC_time_cmp(zRPC_timespec a, zRPC_timespec b);

zRPC_timespec zRPC_time_max(zRPC_timespec a, zRPC_timespec b);

zRPC_timespec zRPC_time_min(zRPC_timespec a, zRPC_timespec b);

/* Add and subtract times.  Calculations saturate at infinities. */
zRPC_timespec zRPC_time_add(zRPC_timespec a, zRPC_timespec b);

zRPC_timespec zRPC_time_sub(zRPC_timespec a, zRPC_timespec b);

/* Return a timespec representing a given number of time units. INT64_MIN is
   interpreted as zRPC_time_inf_past, and INT64_MAX as zRPC_time_inf_future.  */
zRPC_timespec zRPC_time_from_micros(int64_t x, zRPC_clock_type clock_type);

zRPC_timespec zRPC_time_from_nanos(int64_t x, zRPC_clock_type clock_type);

zRPC_timespec zRPC_time_from_millis(int64_t x, zRPC_clock_type clock_type);

zRPC_timespec zRPC_time_from_seconds(int64_t x, zRPC_clock_type clock_type);

zRPC_timespec zRPC_time_from_minutes(int64_t x, zRPC_clock_type clock_type);

zRPC_timespec zRPC_time_from_hours(int64_t x, zRPC_clock_type clock_type);

int32_t zRPC_time_to_millis(zRPC_timespec timespec);

/* Return 1 if two times are equal or within threshold of each other,
   0 otherwise */
int zRPC_time_similar(zRPC_timespec a, zRPC_timespec b,
                      zRPC_timespec threshold);

/* Sleep until at least 'until' - an absolute timeout */
void zRPC_sleep_until(zRPC_timespec until);

double zRPC_timespec_to_micros(zRPC_timespec t);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_TIME_H
