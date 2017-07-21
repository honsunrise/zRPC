//
// Created by zhsyourai on 12/26/16.
//

#include <malloc.h>
#include <string.h>
#include "zRPC/support/useful.h"
#include "../include/timer_heap.h"

/* Adjusts a heap so as to move a hole at position i closer to the root,
   until a suitable position is found for element t. Then, copies t into that
   position. This functor is called each time immediately after modifying a
   value in the underlying container, with the offset of the modified element as
   its argument. */
static void adjust_upwards(zRPC_timer **first, uint32_t i, zRPC_timer *t) {
    while (i > 0) {
        uint32_t parent = (uint32_t) (((int) i - 1) / 2);
        if (zRPC_time_cmp(first[parent]->deadline, t->deadline) <= 0) break;
        first[i] = first[parent];
        first[i]->heap_index = i;
        i = parent;
    }
    first[i] = t;
    t->heap_index = i;
}

/* Adjusts a heap so as to move a hole at position i farther away from the root,
   until a suitable position is found for element t.  Then, copies t into that
   position. */
static void adjust_downwards(zRPC_timer **first, uint32_t i, uint32_t length,
                             zRPC_timer *t) {
    for (;;) {
        uint32_t left_child = 1u + 2u * i;
        if (left_child >= length) break;
        uint32_t right_child = left_child + 1;
        uint32_t next_i = right_child < length &&
                          zRPC_time_cmp(first[left_child]->deadline,
                                        first[right_child]->deadline) > 0
                          ? right_child
                          : left_child;
        if (zRPC_time_cmp(t->deadline, first[next_i]->deadline) <= 0) break;
        first[i] = first[next_i];
        first[i]->heap_index = i;
        i = next_i;
    }
    first[i] = t;
    t->heap_index = i;
}

#define SHRINK_MIN_ELEMS 8
#define SHRINK_FULLNESS_FACTOR 2

static void maybe_shrink(zRPC_timer_heap *heap) {
    if (heap->timer_count >= 8 &&
        heap->timer_count <= heap->timer_capacity / SHRINK_FULLNESS_FACTOR / 2) {
        heap->timer_capacity = heap->timer_count * SHRINK_FULLNESS_FACTOR;
        heap->timers =
                realloc(heap->timers, heap->timer_capacity * sizeof(zRPC_timer *));
    }
}

static void note_changed_priority(zRPC_timer_heap *heap, zRPC_timer *timer) {
    uint32_t i = timer->heap_index;
    uint32_t parent = (uint32_t) (((int) i - 1) / 2);
    if (zRPC_time_cmp(heap->timers[parent]->deadline, timer->deadline) > 0) {
        adjust_upwards(heap->timers, i, timer);
    } else {
        adjust_downwards(heap->timers, i, heap->timer_count, timer);
    }
}

void zRPC_timer_heap_init(zRPC_timer_heap *heap) {
    bzero(heap, sizeof(*heap));
}

void zRPC_timer_heap_destroy(zRPC_timer_heap *heap) { free(heap->timers); }

int zRPC_timer_heap_add(zRPC_timer_heap *heap, zRPC_timer *timer) {
    if (heap->timer_count == heap->timer_capacity) {
        heap->timer_capacity =
                MAX(heap->timer_capacity + 1, heap->timer_capacity * 3 / 2);
        heap->timers =
                realloc(heap->timers, heap->timer_capacity * sizeof(zRPC_timer *));
    }
    timer->heap_index = heap->timer_count;
    adjust_upwards(heap->timers, heap->timer_count, timer);
    heap->timer_count++;
    return timer->heap_index == 0;
}

void zRPC_timer_heap_remove(zRPC_timer_heap *heap, zRPC_timer *timer) {
    uint32_t i = timer->heap_index;
    if (i == heap->timer_count - 1) {
        heap->timer_count--;
        maybe_shrink(heap);
        return;
    }
    heap->timers[i] = heap->timers[heap->timer_count - 1];
    heap->timers[i]->heap_index = i;
    heap->timer_count--;
    maybe_shrink(heap);
    note_changed_priority(heap, heap->timers[i]);
}

int zRPC_timer_heap_is_empty(zRPC_timer_heap *heap) {
    return heap->timer_count == 0;
}

zRPC_timer *zRPC_timer_heap_top(zRPC_timer_heap *heap) {
    if (zRPC_timer_heap_is_empty(heap))
        return NULL;
    return heap->timers[0];
}

void zRPC_timer_heap_pop(zRPC_timer_heap *heap) {
    zRPC_timer_heap_remove(heap, zRPC_timer_heap_top(heap));
}