//
// Created by zhswo on 2016/11/23.
//

#include <string.h>
#include <malloc.h>
#include "event_engine.h"
#include "listener.h"

extern const zRPC_event_engine_vtable poll_event_engine_vtable;
extern const zRPC_event_engine_vtable epoll_event_engine_vtable;
extern const zRPC_timer_engine_vtable minheap_timer_engine_vtable;

static const zRPC_event_engine_vtable *g_event_engines[] = {
    &poll_event_engine_vtable,
    &epoll_event_engine_vtable
};

static const zRPC_timer_engine_vtable *g_timer_engines[] = {
    &minheap_timer_engine_vtable,
};

static int _event_type_2_engine_event_type(int type) {
  int res = 0;
  res |= type & EV_CLOSE ? EVE_CLOSE : 0;
  res |= type & EV_READ ? EVE_READ : 0;
  res |= type & EV_WRITE ? EVE_WRITE : 0;
  res |= type & EV_OPEN ? EVE_WRITE : 0;
  res |= type & EV_ERROR ? EVE_ERROR : 0;
  return res;
}

typedef enum ADDITION_TYPE {
  ADDITION_TYPE_CHANNEL = 0x01,
  ADDITION_TYPE_NOTIFY = 0x02,
  ADDITION_TYPE_LISTENER = 0x04,
} ADDITION_TYPE;

typedef struct _addition_info {
  ADDITION_TYPE type;
  union {
    struct {
      zRPC_channel *channel;
      int is_active;
      int is_inactive;
    };
    zRPC_notify *notify;
    zRPC_listener *listener;
  };
} _addition_info;

zRPC_scheduler *zRPC_scheduler_create() {
  zRPC_scheduler *scheduler = malloc(sizeof(zRPC_scheduler));
  memset(scheduler, 0, sizeof(zRPC_scheduler));
  zRPC_mutex_init(&scheduler->global_mutex);
  scheduler->event_engine = g_event_engines[0];
  scheduler->event_engine_context = scheduler->event_engine->initialize();
  scheduler->timer_engine = g_timer_engines[0];
  scheduler->timer_engine_context = scheduler->timer_engine->initialize();
  zRPC_queue_create(&scheduler->event_queue);
  zRPC_notify_create(&scheduler->notify);
  _addition_info *addition_info = malloc(sizeof(_addition_info));
  addition_info->type = ADDITION_TYPE_NOTIFY;
  addition_info->notify = scheduler->notify;
  scheduler->event_engine->add(scheduler->event_engine_context,
                               scheduler->notify->notify_fd[0],
                               addition_info,
                               EVE_READ);
  return scheduler;
}

void zRPC_scheduler_notify(zRPC_scheduler *scheduler) {
  if (scheduler->owner_thread_id != zRPC_thread_current_id() && scheduler->running_loop) {
    zRPC_notify_write(scheduler->notify);
  }
}

static void _notify_listener_change(void *notify_param,
                                    zRPC_event_source *source,
                                    zRPC_list_head *event_listener_list) {
  zRPC_scheduler *scheduler = notify_param;
  IF_TYPE_SAME(zRPC_channel, source) {
    zRPC_channel *channel = (zRPC_channel *) source;
    scheduler->event_engine->modify(scheduler->event_engine_context,
                                 channel->fd,
                                 _event_type_2_engine_event_type(source->attention_event));
  }
  zRPC_scheduler_notify(scheduler);
}

int zRPC_scheduler_register_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  IF_TYPE_SAME(zRPC_channel, source) {
    zRPC_channel *channel = (zRPC_channel *) source;
    source->notify = _notify_listener_change;
    source->notify_param = scheduler;
    _addition_info *addition_info = malloc(sizeof(_addition_info));
    addition_info->type = ADDITION_TYPE_CHANNEL;
    addition_info->channel = channel;
    addition_info->is_active = 0;
    addition_info->is_inactive = 0;
    scheduler->event_engine->add(scheduler->event_engine_context,
                                 channel->fd,
                                 addition_info,
                                 _event_type_2_engine_event_type(source->attention_event));
  } ELSE_IF_TYPE_SAME(zRPC_timer, source) {
    zRPC_timer *timer = (zRPC_timer *) source;
    scheduler->timer_engine->add(scheduler->timer_engine_context, timer);
    zRPC_scheduler_notify(scheduler);
  } ELSE_IF_TYPE_SAME(zRPC_listener, source) {
    zRPC_listener *listener = (zRPC_listener *) source;
    _addition_info *addition_info = malloc(sizeof(_addition_info));
    addition_info->type = ADDITION_TYPE_LISTENER;
    addition_info->listener = listener;
    scheduler->event_engine->add(scheduler->event_engine_context, listener->fd, addition_info, EVE_READ);
  }
  return 0;
}

int zRPC_scheduler_unregister_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  source->notify = NULL;
  source->notify_param = NULL;
  IF_TYPE_SAME(zRPC_channel, source) {
    zRPC_channel *channel = (zRPC_channel *) source;
    _addition_info *addition_info;
    scheduler->event_engine->del(scheduler->event_engine_context, channel->fd, (void **) &addition_info);
    free(addition_info);
    zRPC_scheduler_notify(scheduler);
  } ELSE_IF_TYPE_SAME(zRPC_timer, source) {
    zRPC_timer *timer = (zRPC_timer *) source;
    scheduler->timer_engine->del(scheduler->timer_engine_context, timer);
    zRPC_scheduler_notify(scheduler);
  } ELSE_IF_TYPE_SAME(zRPC_listener, source) {
    zRPC_listener *listener = (zRPC_listener *) source;
    _addition_info *addition_info;
    scheduler->event_engine->del(scheduler->event_engine_context, listener->fd, (void **) &addition_info);
    free(addition_info);
    zRPC_listener_destroy(listener);
    zRPC_scheduler_notify(scheduler);
  }
  return 0;
}

void zRPC_scheduler_outer_event(zRPC_scheduler *scheduler, zRPC_event event) {
  zRPC_event *tmp = malloc(sizeof(zRPC_event));
  *tmp = event;
  zRPC_queue_enqueue(scheduler->event_queue, tmp);
}

void zRPC_scheduler_destroy(zRPC_scheduler *scheduler) {
  zRPC_mutex_destroy(&scheduler->global_mutex);
}

void zRPC_scheduler_run(zRPC_scheduler *scheduler) {
  if (zRPC_mutex_trylock(&scheduler->global_mutex)) {
    if (scheduler->running_loop) {
      return;
    }
    scheduler->running_loop = 1;
    zRPC_mutex_unlock(&scheduler->global_mutex);

    scheduler->owner_thread_id = zRPC_thread_current_id();

    while (scheduler->running_loop) {
      zRPC_timer **timers;
      size_t ntimers;
      int timeout = scheduler->timer_engine->dispatch(scheduler->timer_engine_context, &timers, &ntimers);

      for (int j = 0; j < ntimers; ++j) {
        zRPC_timer *timer = timers[j];
        timer->triggered = 1;
        zRPC_event *tmp = malloc(sizeof(zRPC_event));
        tmp->event_type = EV_TIMER;
        tmp->event_info = timer;
        zRPC_queue_enqueue(scheduler->event_queue, tmp);
      }

      zRPC_event_engine_result **results;
      size_t nresults = 0;
      scheduler->event_engine->dispatch(scheduler->event_engine_context, timeout, &results, &nresults);

      for (int i = 0; i < nresults; ++i) {
        _addition_info *addition_info = results[i]->fd_info;
        if (addition_info->type == ADDITION_TYPE_CHANNEL) {
          if (results[i]->event_type & EVE_READ) {
            zRPC_event *tmp = malloc(sizeof(zRPC_event));
            tmp->event_type = EV_READ;
            tmp->event_info = addition_info->channel;
            zRPC_queue_enqueue(scheduler->event_queue, tmp);
          }

          if (results[i]->event_type & EVE_WRITE) {
            if (addition_info->is_active) {
              zRPC_event *tmp = malloc(sizeof(zRPC_event));
              tmp->event_type = EV_WRITE;
              tmp->event_info = addition_info->channel;
              zRPC_queue_enqueue(scheduler->event_queue, tmp);
            } else {
              addition_info->is_active = 1;
              zRPC_event *tmp = malloc(sizeof(zRPC_event));
              tmp->event_type = EV_OPEN;
              tmp->event_info = addition_info->channel;
              zRPC_queue_enqueue(scheduler->event_queue, tmp);
            }
          }

          if (results[i]->event_type & EVE_CLOSE) {
            zRPC_event *tmp = malloc(sizeof(zRPC_event));
            tmp->event_type = EV_CLOSE;
            tmp->event_info = addition_info->channel;
            zRPC_queue_enqueue(scheduler->event_queue, tmp);
          }

          if (results[i]->event_type & EVE_ERROR) {
            zRPC_event *tmp = malloc(sizeof(zRPC_event));
            tmp->event_type = EV_ERROR;
            tmp->event_info = addition_info->channel;
            zRPC_queue_enqueue(scheduler->event_queue, tmp);
          }
        } else if (addition_info->type == ADDITION_TYPE_NOTIFY) {
          if (results[i]->event_type & EVE_READ) {
            zRPC_event *tmp = malloc(sizeof(zRPC_event));
            tmp->event_type = EV_READ;
            tmp->event_info = addition_info->notify;
            zRPC_queue_enqueue(scheduler->event_queue, tmp);
          }
        } else if (addition_info->type == ADDITION_TYPE_LISTENER) {
          if (results[i]->event_type & EVE_READ) {
            zRPC_event *tmp = malloc(sizeof(zRPC_event));
            tmp->event_type = EV_READ;
            tmp->event_info = addition_info->listener;
            zRPC_queue_enqueue(scheduler->event_queue, tmp);
          }
        }
      }

      zRPC_event_engine_release_result(results, nresults);

      while (!zRPC_queue_is_empty(scheduler->event_queue)) {
        zRPC_event *event;
        zRPC_queue_dequeue(scheduler->event_queue, (void **) &event);
        IF_TYPE_SAME(zRPC_channel, event->event_info) {
          zRPC_channel *channel = event->event_info;
          zRPC_source__emit_event(&channel->source, *event);
        } ELSE_IF_TYPE_SAME(zRPC_timer, event->event_info) {
          zRPC_timer *timer = event->event_info;
          zRPC_source__emit_event(&timer->source, *event);
        } ELSE_IF_TYPE_SAME(zRPC_notify, event->event_info) {
          zRPC_notify *notify = event->event_info;
          zRPC_source__emit_event(&notify->source, *event);
        } ELSE_IF_TYPE_SAME(zRPC_listener, event->event_info) {
          zRPC_listener *listener = event->event_info;
          zRPC_source__emit_event(&listener->source, *event);
        }
        free(event);
      }
    }
  };
}