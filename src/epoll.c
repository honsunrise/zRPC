//
// Created by zhswo on 2016/11/23.
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "zRPC/event_engine.h"
#include "zRPC/ds/hashmap.h"

static void *initialize();

static int add(void *engine_context, zRPC_event *event);

static int del(void *engine_context, zRPC_event *event);

static int dispatch(void *engine_context, uint32_t timeout, zRPC_event *events[], size_t *nevents);

static void release(void *engine_context);

static int int_hash_function(void *key) {
  return (int) (key);
}

static int equals(void *keyA, void *keyB) {
  return keyA == keyB;
}

typedef struct zRPC_epoll_context {
  int ep_fd;
  zRPC_hashmap *ep_evs;
} zRPC_epoll_context;

const zRPC_event_engine_vtable epoll_event_engine_vtable = {
    "epoll",
    initialize,
    add,
    del,
    dispatch,
    release,
    sizeof(zRPC_epoll_context)
};

static void *initialize() {
  zRPC_epoll_context *epoll_context = malloc(sizeof(zRPC_epoll_context));
  epoll_context->ep_fd = epoll_create1(EPOLL_CLOEXEC);
  epoll_context->ep_evs = hashmapCreate(1024, int_hash_function, equals);
  return epoll_context;
}

static void release(void *engine_context) {
  if (engine_context) {
    hashmapFree(((zRPC_epoll_context *) engine_context)->ep_evs);
    close(((zRPC_epoll_context *) engine_context)->ep_fd);
    free(engine_context);
  }
}

static int add(void *engine_context, zRPC_event *event) {
  zRPC_epoll_context *epoll_context = engine_context;
  if (!(event->event_status & EVS_INIT)) {
    return -1;
  }
  struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd));
  if (evs == NULL) {
    evs = malloc(sizeof(struct epoll_event));
    evs->data.ptr = event;
    evs->events = EPOLLET | EPOLLRDHUP | EPOLLERR;
    if (event->event_type & EV_WRITE) {
      evs->events |= EPOLLOUT;
    }
    if (event->event_type & EV_READ) {
      evs->events |= EPOLLIN;
    }
    hashmapPut(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd), evs);
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_ADD, zRPC_fd_origin(event->fd), evs);
  } else {
    if (event->event_type & EV_WRITE) {
      evs->events |= EPOLLOUT;
    }
    if (event->event_type & EV_READ) {
      evs->events |= EPOLLIN;
    }
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, zRPC_fd_origin(event->fd), evs);
  }
  event->event_status = EVS_REGISTER;
  return 0;
}

static int del(void *engine_context, zRPC_event *event) {
  zRPC_epoll_context *epoll_context = engine_context;
  if (!(event->event_status & EVS_REGISTER)) {
    return -1;
  }
  struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd));
  if (event->event_type & EV_READ)
    evs->events &= ~EPOLLIN;
  if (event->event_type & EV_WRITE)
    evs->events &= ~EPOLLOUT;
  if (evs->events & (EPOLLIN | EPOLLOUT)) {
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, zRPC_fd_origin(event->fd), evs);
    return 0;
  } else {
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_DEL, zRPC_fd_origin(event->fd), NULL);
    return 0;
  }
}

#define MAX_EVENTS 128

static int dispatch(void *engine_context, uint32_t timeout, zRPC_event *events[], size_t *nevents_out) {
  zRPC_epoll_context *epoll_context = engine_context;
  struct epoll_event ep_events[MAX_EVENTS];
  int ep_rv = epoll_wait(epoll_context->ep_fd, ep_events, MAX_EVENTS, timeout);
  if (ep_rv == -1) {
    if (errno != EINTR || errno != ETIMEDOUT)
      return -1;
    return 0;
  }

  for (int i = 0, j = 0; i < ep_rv; ++i) {
    zRPC_event *event = ep_events[i].data.ptr;
    int close = ep_events[i].events & (EPOLLRDHUP);
    int error = ep_events[i].events & (EPOLLERR | EPOLLHUP);
    int read_ev = ep_events[i].events & (EPOLLIN);
    int write_ev = ep_events[i].events & EPOLLOUT;
    int res = 0;
    if (read_ev) {
      res |= EV_READ;
    }
    if (write_ev) {
      res |= EV_WRITE;
    }
    if (error) {
      res |= EV_ERROR;
    }
    if (close) {
      res |= EV_CLOSE;
    }
    if (res == 0) {
      continue;
    }
    events[j++] = event;
    *nevents_out = (size_t) j;
  }
  return 0;
}