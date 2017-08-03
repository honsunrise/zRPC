//
// Created by zhsyourai on 12/26/16.
//

#ifndef ZRPC_TIMER_HEAP_H
#define ZRPC_TIMER_HEAP_H

#include <stddef.h>
#include <stdint.h>
#include "zRPC/timer.h"

typedef struct zRPC_timer_heap {
  zRPC_timer_task **timers;
    uint32_t timer_count;
    uint32_t timer_capacity;
} zRPC_timer_heap;

/* return 1 if the new timer is the first timer in the heap */
int zRPC_timer_heap_add(zRPC_timer_heap *heap, zRPC_timer_task *task);

void zRPC_timer_heap_init(zRPC_timer_heap *heap);

void zRPC_timer_heap_destroy(zRPC_timer_heap *heap);

void zRPC_timer_heap_remove(zRPC_timer_heap *heap, zRPC_timer_task *task);

zRPC_timer_task *zRPC_timer_heap_top(zRPC_timer_heap *heap);

void zRPC_timer_heap_pop(zRPC_timer_heap *heap);

int zRPC_timer_heap_is_empty(zRPC_timer_heap *heap);

#endif //ZRPC_TIMER_HEAP_H
