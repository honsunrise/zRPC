//
// Created by zhsyourai on 12/27/16.
//

#ifndef ZRPC_TIMER_H
#define ZRPC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "time.h"
#include "runnable.h"

typedef struct zRPC_timer {
    zRPC_timespec deadline;
    uint32_t heap_index; /* INVALID_HEAP_INDEX if not in heap */
    int triggered;
    struct zRPC_timer *next;
    struct zRPC_timer *prev;
    zRPC_runnable *runnable;
} zRPC_timer;

typedef struct zRPC_timer_holder zRPC_timer_holder;

struct zRPC_context;

zRPC_timer *zRPC_timer_schedule(struct zRPC_context *context, zRPC_timespec deadline, zRPC_runnable *runnable);

zRPC_timer *zRPC_timer_schedule_now(struct zRPC_context *context, zRPC_timespec deadline, zRPC_runnable *runnable,
                                    zRPC_timespec now);

void zRPC_timer_canael(struct zRPC_context *context, zRPC_timer *timer);

void zRPC_timer_init(struct zRPC_context *context);

void zRPC_timer_shutdown(struct zRPC_context *context);

void zRPC_timer_next_timeout(struct zRPC_context *context, zRPC_timespec *ts);

int zRPC_timer_run_some_expired_timers(struct zRPC_context *context);

int zRPC_timer_run_some_expired_timers_now(struct zRPC_context *context, zRPC_timespec now);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_TIMER_H
