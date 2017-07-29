//
// Created by zhsyourai on 12/27/16.
//

#include "support/lock.h"
#include "zRPC/timer.h"
#include "zRPC/scheduling.h"

zRPC_timer *zRPC_timer_create(struct zRPC_scheduler *scheduler, zRPC_timespec deadline) {
  zRPC_timer *timer = malloc(sizeof(zRPC_timer));
  RTTI_INIT_PTR(zRPC_timer, &timer->source);
  zRPC_source_init(&timer->source);
  timer->deadline = deadline;
  timer->triggered = 0;
  return timer;
}

void zRPC_timer_destroy(zRPC_scheduler *scheduler, zRPC_timer *timer) {
  zRPC_scheduler_unregister_source(scheduler, &timer->source);
}