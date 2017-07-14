//
// Created by zhswo on 2016/11/23.
//

#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <unistd.h>
#include "support/fd_notifiable.h"
#include "zRPC/event.h"
#include "zRPC/channel.h"

extern const zRPC_event_engine_vtable poll_event_engine_vtable;
extern const zRPC_event_engine_vtable epoll_event_engine_vtable;

static const zRPC_event_engine_vtable *g_event_engines[] = {
    &poll_event_engine_vtable,
    &epoll_event_engine_vtable
};

static void notify_cb(zRPC_context *context) {
  unsigned char buf[8];
  struct zRPC_context *base = context;
  while (read(zRPC_fd_origin(context->notify_fd[0]), (char *) buf, sizeof(buf)) > 0);
  base->is_notify_pending = 0;
}

zRPC_context *zRPC_context_create() {
  zRPC_context *context = malloc(sizeof(zRPC_context));
  memset(context, 0, sizeof(zRPC_context));
  zRPC_mutex_init(&context->global_mutex);
  zRPC_cond_init(&context->global_cond);

  zRPC_list_init(&context->event_register);
  zRPC_list_init(&context->event_active);
  zRPC_list_init(&context->event_pending);
  zRPC_list_init(&context->event_remove);

  context->event_engine = g_event_engines[1];
  context->event_engine_context = context->event_engine->initialize(context);
  context->exit_dispatch = 0;
  zRPC_timer_init(context);
  zRPC_create_notifiable_fd(context->notify_fd);
  context->notify_runnable = zRPC_runnable_create((void *(*)(void *)) notify_cb, context,
                                                  zRPC_runnable_noting_callback);
  context->notify_event = zRPC_event_create(context->notify_fd[0], EV_READ | EV_PERSIST, context->notify_runnable);
  zRPC_context_register_event(context, context->notify_event);

  zRPC_resolver_init(context);
  return context;
}

void zRPC_context_notify(zRPC_context *context) {
  if (context->is_notify_pending
      || context->owner_thread_id == zRPC_thread_current_id()) {
    return;
  }
  char buf[1];
  buf[0] = (char) 0;
  context->is_notify_pending = 1;
  write(zRPC_fd_origin(context->notify_fd[1]), buf, 1);
}

int zRPC_context_register_event(zRPC_context *context, zRPC_event *event) {
  if (!(event->event_status & EVS_INIT)) {
    return -1;
  }
  if (event->event_type & EVENT_TYPE_FD_MASK && event->fd) {
    context->event_engine->add(context, event);
  }
  zRPC_list_add_tail(&event->list_node_register, &context->event_register);
  zRPC_context_notify(context);
  return 0;
}

int zRPC_context_unregister_event(zRPC_context *context, zRPC_event *event) {
  if (!(event->event_status & EVS_REGISTER)) {
    return -1;
  }
  if (event->event_type & EVENT_TYPE_FD_MASK && event->fd) {
    context->event_engine->del(context, event);
  }
  zRPC_list_del(&event->list_node_register);
  zRPC_context_notify(context);
  return 0;
}

size_t zRPC_context_unregister_event_fd(zRPC_context *context, zRPC_sample_fd *fd) {
  volatile size_t ret = 0;
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &context->event_register) {
    zRPC_event *event = zRPC_list_entry(pos, zRPC_event, list_node_register);
    if (event->fd == fd) {
      zRPC_list_add(&event->list_node_remove, &context->event_remove);
    }
  }
  zRPC_list_for_each(pos, &context->event_remove) {
    zRPC_event *event = zRPC_list_entry(pos, zRPC_event, list_node_remove);
    zRPC_list_del(&event->list_node_register);
    ++ret;
  }
  zRPC_list_init(&context->event_remove);
  return ret;
}

void zRPC_context_fd_event_happen(zRPC_context *context, zRPC_sample_fd *fd, int res) {
  zRPC_event *event = NULL;
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &context->event_register) {
    event = zRPC_list_entry(pos, zRPC_event, list_node_register);
    if (event->event_status == EVS_REGISTER && event->fd == fd && event->event_type & res) {
      break;
    } else {
      if (event->fd == fd && event->event_type & res) {
        zRPC_pending_event *pending_event = malloc(sizeof(zRPC_pending_event));
        pending_event->event = event;
        pending_event->event_happen = res;
        zRPC_list_add(&pending_event->list_node, &context->event_pending);
        return;
      }
    }
  }
  if (event == NULL) {
    return;
  }
  zRPC_pending_event *pending_event = NULL;
  zRPC_list_for_each(pos, &context->event_pending) {
    pending_event = zRPC_list_entry(pos, zRPC_pending_event, list_node);
    if (pending_event->event == event) {
      event->event_happen |= pending_event->event_happen;
      free(pending_event);
    }
  }
  zRPC_list_init(&context->event_pending);
  event->event_happen |= res;
  event->event_status = EVS_ACTIVE;
  ++context->event_active_count;
  zRPC_list_add_tail(&event->list_node_active, &context->event_active);
}

void zRPC_context_timer_event_happen(zRPC_context *context, zRPC_timer *timer, int res) {
  zRPC_event *event = NULL;
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &context->event_register) {
    event = zRPC_list_entry(pos, zRPC_event, list_node_register);
    if (event->event_status == EVS_REGISTER && event->timer == timer) {
      break;
    }
  }
  if (event == NULL) {
    return;
  }
  event->event_happen |= res;
  event->event_status = EVS_ACTIVE;
  ++context->event_active_count;
  zRPC_list_add_tail(&event->list_node_active, &context->event_active);
}

void zRPC_context_destroy(zRPC_context *context) {
  zRPC_cond_destroy(&context->global_cond);
  zRPC_mutex_destroy(&context->global_mutex);
}

static void zRPC_context_run(zRPC_context *context, zRPC_runnable *runnable) {
  zRPC_runnable_run(runnable);
}

void zRPC_context_dispatch(zRPC_context *context) {
  if (context->running_loop) {
    return;
  }

  context->running_loop = 1;
  context->owner_thread_id = zRPC_thread_current_id();
  // clear time cache
  context->ts_cache.tv_sec = context->ts_cache.tv_nsec = 0;

  for (;;) {
    if (context->exit_dispatch) {
      return;
    }
    zRPC_timespec ts = zRPC_time_0(zRPC_TIMESPAN);
    if (!context->event_active_count) {
      zRPC_timer_next_timeout(context, &ts);
    }

    context->event_engine->dispatch(context, &ts);
    zRPC_list_head *pos;
    zRPC_event *event;

    // update cache time
    context->ts_cache = zRPC_now(zRPC_CLOCK_MONOTONIC);

    // process time out
    zRPC_timer_run_some_expired_timers(context);

    zRPC_list_for_each(pos, &context->event_active) {
      event = zRPC_list_entry(pos, zRPC_event, list_node_active);
      --context->event_active_count;
      int happen = event->event_type & event->event_happen;
      event->event_happen = 0;
      if (happen == 0) {
        continue;
      }
      if (event->event_type & EVENT_TYPE_FD_MASK) {
        int read_ev = happen & (EV_READ);
        int write_ev = happen & (EV_WRITE);
        int error_ev = happen & (EV_ERROR);
        if (read_ev || write_ev || error_ev) {
          zRPC_context_run(context, event->callback);
        }
        event->event_status = EVS_REGISTER;
        if (!(event->event_type & EV_PERSIST)) {
          zRPC_context_unregister_event(context, event);
          zRPC_event_destroy(event);
        }
      } else if (event->event_type & EVENT_TYPE_TIMER_MASK) {
        if (happen & EV_TIMER || happen & EV_ERROR) {
          zRPC_context_run(context, event->callback);
        }
        event->event_status = EVS_REGISTER;
        if (!(event->event_type & EV_PERSIST)) {
          zRPC_context_unregister_event(context, event);
          zRPC_event_destroy(event);
        }
      }
    }
    zRPC_list_init(&context->event_active);
  }
}