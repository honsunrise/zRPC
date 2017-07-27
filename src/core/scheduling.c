//
// Created by zhswo on 2016/11/23.
//

#include <string.h>
#include <malloc.h>
#include "zRPC/scheduling.h"
#include "channel.h"

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

static int _engine_event_type_2_event_type(int type) {
  int res = 0;
  res |= type & EVE_CLOSE ? EV_CLOSE : 0;
  res |= type & EVE_READ ? EV_READ : 0;
  res |= type & EVE_WRITE ? EV_WRITE : 0;
  res |= type & EVE_ERROR ? EVE_ERROR : 0;
  return res;
}

typedef struct _channel_status {
  zRPC_channel *channel;
  int is_active;
  int is_inactive;
} _channel_status;

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
  zRPC_scheduler_register_source(scheduler, &scheduler->notify->source);
  return scheduler;
}

void zRPC_scheduler_notify(zRPC_scheduler *scheduler) {
  if (scheduler->owner_thread_id == zRPC_thread_current_id() && scheduler->running_loop) {
    zRPC_notify_write(scheduler->notify);
  }
}

static void _notify_listener_change(void *notify_param,
                                    zRPC_event_source *source,
                                    zRPC_list_head *event_listener_list) {
  zRPC_scheduler *scheduler = notify_param;
  IF_TYPE_SAME(zRPC_channel, source) {
    zRPC_channel *channel = (zRPC_channel *) source;
    _channel_status *channel_status = malloc(sizeof(_channel_status));
    channel_status->channel = channel;
    channel_status->is_active = 0;
    channel_status->is_inactive = 0;
    scheduler->event_engine->set(scheduler->event_engine_context,
                                 channel->fd,
                                 channel_status,
                                 _event_type_2_engine_event_type(source->attention_event));
  } ELSE_IF_TYPE_SAME(zRPC_timer, source) {

  }
  zRPC_scheduler_notify(scheduler);
}

int zRPC_scheduler_register_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  source->notify = _notify_listener_change;
  source->notify_param = scheduler;
  return 0;
}

int zRPC_scheduler_unregister_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  source->notify = NULL;
  source->notify_param = NULL;
  IF_TYPE_SAME(zRPC_channel, source) {
    zRPC_channel *channel = (zRPC_channel *) source;
    _channel_status *channel_status;
    scheduler->event_engine->del(scheduler->event_engine_context, channel->fd, (void **) &channel_status);
    zRPC_channel_destroy(channel);
  } ELSE_IF_TYPE_SAME(zRPC_timer, source) {

  }
  return 0;
}

void zRPC_schedular_outer_event(zRPC_scheduler *scheduler, zRPC_event event) {
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
    // clear time cache
    scheduler->ts_cache.tv_sec = scheduler->ts_cache.tv_nsec = 0;

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
      size_t nresults;
      scheduler->event_engine->dispatch(scheduler->event_engine_context, timeout, &results, &nresults);

      for (int i = 0; i < nresults; ++i) {
        _channel_status *channel_status = results[i]->fd_info;
        if (_engine_event_type_2_event_type(results[i]->event_type) & EV_READ) {
          zRPC_event *tmp = malloc(sizeof(zRPC_event));
          tmp->event_type = EV_READ;
          tmp->event_info = channel_status->channel;
          zRPC_queue_enqueue(scheduler->event_queue, tmp);
        }

        if (_engine_event_type_2_event_type(results[i]->event_type) & EV_WRITE) {
          zRPC_event *tmp = malloc(sizeof(zRPC_event));
          tmp->event_type = EV_WRITE;
          tmp->event_info = channel_status->channel;
          zRPC_queue_enqueue(scheduler->event_queue, tmp);
        }

        if (_engine_event_type_2_event_type(results[i]->event_type) & EV_CLOSE) {
          zRPC_event *tmp = malloc(sizeof(zRPC_event));
          tmp->event_type = EV_CLOSE;
          tmp->event_info = channel_status->channel;
          zRPC_queue_enqueue(scheduler->event_queue, tmp);
        }

        if (_engine_event_type_2_event_type(results[i]->event_type) & EV_ERROR) {
          zRPC_event *tmp = malloc(sizeof(zRPC_event));
          tmp->event_type = EV_ERROR;
          tmp->event_info = channel_status->channel;
          zRPC_queue_enqueue(scheduler->event_queue, tmp);
        }
      }
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
          zRPC_timer *timer = event->event_info;
          zRPC_source__emit_event(&timer->source, *event);
        }
        free(event);
      }
    }
  };
}