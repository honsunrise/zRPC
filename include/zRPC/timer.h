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

typedef struct zRPC_timer {
  zRPC_event_source source;
  zRPC_timespec deadline;
  int triggered;
  uint32_t heap_index; /* INVALID_HEAP_INDEX if not in heap */
} zRPC_timer;

struct zRPC_scheduler;

zRPC_timer *zRPC_timer_create(struct zRPC_scheduler *scheduler, zRPC_timespec deadline);

void zRPC_timer_destroy(struct zRPC_scheduler *scheduler, zRPC_timer *timer);


#ifdef __cplusplus
}
#endif
#endif //ZRPC_TIMER_H
