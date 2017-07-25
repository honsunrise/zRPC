//
// Created by zhsyourai on 12/27/16.
//

#include "zRPC/support/lock.h"
#include "zRPC/timer.h"
#include "zRPC/scheduling.h"
#include "../include/timer_heap.h"
#include "../include/rtti.h"

struct zRPC_timer_holder {
  zRPC_mutex mutex;
  zRPC_mutex run_mutex;
  zRPC_timer_heap heap;
  int initialized;
  zRPC_timespec min_deadline;
};

zRPC_timer *zRPC_timer_schedule(zRPC_scheduler *context, zRPC_timespec deadline, zRPC_runnable *runnable) {
  return zRPC_timer_schedule_now(context, deadline, runnable, zRPC_now(zRPC_CLOCK_MONOTONIC));
}

zRPC_timer *zRPC_timer_schedule_now(zRPC_scheduler *context, zRPC_timespec deadline, zRPC_runnable *runnable,
                                    zRPC_timespec now) {
  zRPC_timer *timer = malloc(sizeof(zRPC_timer));
  zRPC_event *event = zRPC_event_create(timer, EV_TIMER, runnable);
  zRPC_context_register_event(context, event);
  int is_first_timer = 0;
  timer->deadline = deadline;
  timer->runnable = runnable;
  timer->triggered = 0;
  if (!context->timer_holder->initialized) {
    timer->triggered = 1;
    zRPC_context_timer_event_happen(context, timer, EV_TIMER);
    zRPC_context_notify(context);
    return timer;
  }
  if (zRPC_time_cmp(deadline, now) <= 0) {
    timer->triggered = 1;
    zRPC_context_timer_event_happen(context, timer, EV_TIMER);
    zRPC_context_notify(context);
    return timer;
  }
  zRPC_mutex_lock(&context->timer_holder->mutex);
  is_first_timer = zRPC_timer_heap_add(&context->timer_holder->heap, timer);
  zRPC_mutex_unlock(&context->timer_holder->mutex);
  if (is_first_timer) {
    zRPC_mutex_lock(&context->timer_holder->mutex);
    if (zRPC_time_cmp(deadline, context->timer_holder->min_deadline) < 0) {
      context->timer_holder->min_deadline = deadline;
    }
    zRPC_mutex_unlock(&context->timer_holder->mutex);
  }
  return timer;
}

void zRPC_timer_canael(zRPC_scheduler *context, zRPC_timer *timer) {
  if (!context->timer_holder->initialized) {
    return;
  }

  zRPC_mutex_lock(&context->timer_holder->mutex);
  if (!timer->triggered) {
    zRPC_context_timer_event_happen(context, timer, EV_TIMER);
    timer->triggered = 1;
    zRPC_timer_heap_remove(&context->timer_holder->heap, timer);
  }
  zRPC_mutex_unlock(&context->timer_holder->mutex);
}

void zRPC_timer_init(zRPC_scheduler *context) {
  context->timer_holder = malloc(sizeof(zRPC_timer_holder));
  zRPC_timer_heap_init(&context->timer_holder->heap);
  zRPC_mutex_init(&context->timer_holder->mutex);
  zRPC_mutex_init(&context->timer_holder->run_mutex);
  context->timer_holder->min_deadline = zRPC_time_inf_future(zRPC_CLOCK_MONOTONIC);
  context->timer_holder->initialized = 1;
}

void zRPC_timer_shutdown(zRPC_scheduler *context) {
  zRPC_timer_heap_destroy(&context->timer_holder->heap);
  zRPC_mutex_destroy(&context->timer_holder->mutex);
  zRPC_mutex_destroy(&context->timer_holder->run_mutex);
  context->timer_holder->initialized = 0;
}

void zRPC_timer_next_timeout(zRPC_scheduler *context, zRPC_timespec *ts) {
  int res = 0;

  zRPC_timer *timer = zRPC_timer_heap_top(&context->timer_holder->heap);

  if (timer == NULL) {
    *ts = zRPC_time_inf_future(zRPC_TIMESPAN);
    return;
  }

  zRPC_timespec now = zRPC_now(zRPC_CLOCK_MONOTONIC);

  if (zRPC_time_cmp(timer->deadline, now) < 0) {
    *ts = zRPC_time_0(zRPC_TIMESPAN);
    return;
  }

  *ts = zRPC_time_sub(timer->deadline, now);
}

static double ts_to_dbl(zRPC_timespec ts) {
  return (double) ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static zRPC_timespec dbl_to_ts(double d) {
  zRPC_timespec ts;
  ts.tv_sec = (int64_t) d;
  ts.tv_nsec = (int32_t) (1e9 * (d - (double) ts.tv_sec));
  ts.clock_type = zRPC_TIMESPAN;
  return ts;
}

static void list_join(zRPC_timer *head, zRPC_timer *timer) {
  timer->next = head;
  timer->prev = head->prev;
  timer->next->prev = timer->prev->next = timer;
}

static void list_remove(zRPC_timer *timer) {
  timer->next->prev = timer->prev;
  timer->prev->next = timer->next;
}

static zRPC_timer *pop_one(zRPC_scheduler *context, zRPC_timespec now) {
  zRPC_timer *timer;
  for (;;) {
    if (zRPC_timer_heap_is_empty(&context->timer_holder->heap))
      return NULL;
    timer = zRPC_timer_heap_top(&context->timer_holder->heap);
    if (zRPC_time_cmp(timer->deadline, now) > 0) return NULL;
    timer->triggered = 1;
    zRPC_timer_heap_pop(&context->timer_holder->heap);
    context->timer_holder->min_deadline = timer->deadline;
    return timer;
  }
}

static size_t pop_timers(zRPC_scheduler *context, zRPC_timespec now) {
  size_t n = 0;
  zRPC_timer *timer;
  while ((timer = pop_one(context, now), timer)) {
    zRPC_context_timer_event_happen(context, timer, EV_TIMER);
    n++;
  }
  return n;
}

int zRPC_timer_run_some_expired_timers_now(zRPC_scheduler *context, zRPC_timespec now) {
  size_t n = 0;
  if (zRPC_mutex_trylock(&context->timer_holder->run_mutex)) {
    zRPC_mutex_lock(&context->timer_holder->mutex);
    while (zRPC_time_cmp(context->timer_holder->min_deadline, now) < 0) {
      size_t ret_n = pop_timers(context, now);
      if (ret_n == 0) {
        zRPC_mutex_unlock(&context->timer_holder->mutex);
        zRPC_mutex_unlock(&context->timer_holder->run_mutex);
        return 0;
      }
      n += ret_n;
    }
    zRPC_mutex_unlock(&context->timer_holder->mutex);
    zRPC_mutex_unlock(&context->timer_holder->run_mutex);

  }
  return (int) n;
}

int zRPC_timer_run_some_expired_timers(zRPC_scheduler *context) {
  return zRPC_timer_run_some_expired_timers_now(context, zRPC_now(zRPC_CLOCK_MONOTONIC));
}