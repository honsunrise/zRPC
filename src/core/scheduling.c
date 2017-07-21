//
// Created by zhswo on 2016/11/23.
//

#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <unistd.h>
#include "rtti.h"
#include "../include/fd_notifiable.h"
#include "zRPC/channel.h"

extern const zRPC_event_engine_vtable poll_event_engine_vtable;
extern const zRPC_event_engine_vtable epoll_event_engine_vtable;

static const zRPC_event_engine_vtable *g_event_engines[] = {
    &poll_event_engine_vtable,
    &epoll_event_engine_vtable
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

static void notify_cb(zRPC_scheduler *scheduler) {
  unsigned char buf[8];
  struct zRPC_scheduler *base = scheduler;
  while (read(zRPC_fd_origin(scheduler->notify_fd[0]), (char *) buf, sizeof(buf)) > 0);
  base->is_notify_pending = 0;
}

zRPC_scheduler *zRPC_scheduler_create() {
  zRPC_scheduler *scheduler = malloc(sizeof(zRPC_scheduler));
  memset(scheduler, 0, sizeof(zRPC_scheduler));
  zRPC_mutex_init(&scheduler->global_mutex);
  zRPC_cond_init(&scheduler->global_cond);

  zRPC_list_init(&scheduler->source_list);
  zRPC_list_init(&scheduler->source_active);

  scheduler->event_engine = g_event_engines[0];
  scheduler->event_engine_context = scheduler->event_engine->initialize();
  zRPC_timer_init(scheduler);
  zRPC_create_notifiable_fd(scheduler->notify_fd);
  scheduler->notify_runnable = zRPC_runnable_create((void *(*)(void *)) notify_cb, scheduler,
                                                    zRPC_runnable_noting_callback);

  zRPC_resolver_init(scheduler);
  return scheduler;
}

void zRPC_scheduler_notify(zRPC_scheduler *scheduler) {
  if (scheduler->is_notify_pending
      || scheduler->owner_thread_id == zRPC_thread_current_id()) {
    return;
  }
  char buf[1];
  buf[0] = (char) 0;
  scheduler->is_notify_pending = 1;
  write(zRPC_fd_origin(scheduler->notify_fd[1]), buf, 1);
}

static void _notify_listener_change(void *notify_param,
                                    zRPC_event_source *source,
                                    zRPC_list_head *event_listener_list) {
  zRPC_scheduler *scheduler = notify_param;
  IF_TYPE_SAME(zRPC_fd, source) {
    zRPC_fd *fd = (zRPC_fd *) source;
    scheduler->event_engine->set(scheduler->event_engine_context,
                                 zRPC_fd_origin(fd),
                                 fd,
                                 _event_type_2_engine_event_type(source->attention_event));
  }
}

int zRPC_scheduler_register_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  source->notify = _notify_listener_change;
  source->notify_param = scheduler;
  zRPC_list_add_tail(&(source)->source_list_node, &scheduler->source_list);
  return 0;
}

int zRPC_scheduler_unregister_source(zRPC_scheduler *scheduler, zRPC_event_source *source) {
  IF_TYPE_SAME(zRPC_fd, source) {
    zRPC_fd *fd = (zRPC_fd *) source;
    scheduler->event_engine->del(scheduler->event_engine_context, zRPC_fd_origin(fd));
  }
  source->notify = NULL;
  source->notify_param = NULL;
  zRPC_list_del(&(source)->source_list_node);
  return 0;
}

void zRPC_scheduler_event_happen(zRPC_scheduler *scheduler, zRPC_event *event) {
  zRPC_event_source *source = NULL;
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &scheduler->source_list) {
    source = zRPC_list_entry(pos, zRPC_event_source, source_list_node);
    if (event->event_status == EVS_REGISTER && event->fd == fd && event->event_type & res) {
      break;
    } else {
      if (event->fd == fd && event->event_type & res) {
        zRPC_pending_event *pending_event = malloc(sizeof(zRPC_pending_event));
        pending_event->event = event;
        pending_event->event_happen = res;
        zRPC_list_add(&pending_event->list_node, &scheduler->event_pending);
        return;
      }
    }
  }
  if (event == NULL) {
    return;
  }
  zRPC_pending_event *pending_event = NULL;
  zRPC_list_for_each(pos, &scheduler->event_pending) {
    pending_event = zRPC_list_entry(pos, zRPC_pending_event, list_node);
    if (pending_event->event == event) {
      event->event_happen |= pending_event->event_happen;
      free(pending_event);
    }
  }
  zRPC_list_init(&scheduler->event_pending);
  event->event_happen |= res;
  event->event_status = EVS_ACTIVE;
  ++scheduler->event_active_count;
  zRPC_list_add_tail(&event->list_node_active, &scheduler->event_active);
}

void zRPC_scheduler_destroy(zRPC_scheduler *scheduler) {
  zRPC_cond_destroy(&scheduler->global_cond);
  zRPC_mutex_destroy(&scheduler->global_mutex);
}

void zRPC_scheduler_run(zRPC_scheduler *scheduler) {
  if (scheduler->running_loop) {
    return;
  }

  scheduler->running_loop = 1;
  scheduler->owner_thread_id = zRPC_thread_current_id();
  // clear time cache
  scheduler->ts_cache.tv_sec = scheduler->ts_cache.tv_nsec = 0;

  for (;;) {
    zRPC_timespec ts = zRPC_time_0(zRPC_TIMESPAN);
    zRPC_timer_next_timeout(scheduler, &ts);
    zRPC_event_engine_result *results;
    size_t nresults;
    scheduler->event_engine->dispatch(scheduler, zRPC_time_to_millis(ts), &results, &nresults);
    zRPC_list_head *pos;
    zRPC_event_source *source;

    // update cache time
    scheduler->ts_cache = zRPC_now(zRPC_CLOCK_MONOTONIC);

    // process time out
    zRPC_timer_run_some_expired_timers(scheduler);

    for (int i = 0; i < nresults; ++i) {
      if (source->attention_event & _engine_event_type_2_event_type(results[i].event_type)) {
        zRPC_list_for_each(pos, &scheduler->source_list) {
          source = zRPC_list_entry(pos, zRPC_event_source, source_list_node);

        }
      }
    }
  }
}