//
// Created by zhsyourai on 12/27/16.
//

#ifndef ZRPC_TIMER_H
#define ZRPC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "support/time.h"
#include "support/runnable.h"
#include "source.h"

typedef void (*zPRC_timer_callback)(zRPC_timespec deadline, void *param);

typedef struct zRPC_timer {
  zRPC_event_source source;
  struct zRPC_scheduler *scheduler;
  zRPC_list_head task_list;
} zRPC_timer;

typedef struct zRPC_timer_task {
  zRPC_timer *timer;
  zRPC_list_head node;
  zRPC_timespec deadline;
  zPRC_timer_callback callback;
  zRPC_event_listener *listener;
  uint32_t heap_index; /* INVALID_HEAP_INDEX if not in heap */
  int triggered;
  void *param;
} zRPC_timer_task;

struct zRPC_scheduler;

zRPC_timer *zRPC_timer_create(struct zRPC_scheduler *scheduler);

zRPC_timer_task *zRPC_timer_timeout(zRPC_timer *timer,
                                    zRPC_timespec timeout,
                                    zPRC_timer_callback callback,
                                    void *param);

zRPC_timer_task *zRPC_timer_deadline(zRPC_timer *timer,
                                     zRPC_timespec deadline,
                                     zPRC_timer_callback callback,
                                     void *param);

void zRPC_timer_destroy(zRPC_timer *timer);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_TIMER_H
