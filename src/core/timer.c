//
// Created by zhsyourai on 12/27/16.
//

#include "zRPC/support/lock.h"
#include "zRPC/timer.h"
#include "zRPC/scheduling.h"

zRPC_timer *zRPC_timer_schedule(zRPC_scheduler *context, zRPC_timespec deadline) {
  return zRPC_timer_schedule_now(context, deadline, zRPC_now(zRPC_CLOCK_MONOTONIC));
}

zRPC_timer *zRPC_timer_schedule_now(zRPC_scheduler *scheduler, zRPC_timespec deadline, zRPC_timespec now) {
  zRPC_timer *timer = malloc(sizeof(zRPC_timer));
  RTTI_INIT_PTR(zRPC_timer, &timer->source);
  zRPC_source_init(&timer->source);
  timer->deadline = deadline;
  timer->triggered = 0;
  zRPC_scheduler_register_source(scheduler, &timer->source);
  return timer;
}

void zRPC_timer_cancel(zRPC_scheduler *scheduler, zRPC_timer *timer) {
  zRPC_scheduler_unregister_source(scheduler, &timer->source);
}