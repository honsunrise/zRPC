//
// Created by zhswo on 2016/11/23.
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "event_engine.h"
#include "ds/hashmap.h"

static void *initialize();

static int add(void *engine_context, int fd, void *fd_info, int event_type);

static int modify(void *engine_context, int fd, int event_type);

static int del(void *engine_context, int fd, void **fd_info);

static int dispatch(void *engine_context, int32_t timeout, zRPC_event_engine_result **results[], size_t *nresults);

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
    modify,
    del,
    dispatch,
    release
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

static int add(void *engine_context, int fd, void *fd_info, int event_type) {
  zRPC_epoll_context *epoll_context = engine_context;
  struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) fd);
  if (evs == NULL) {
    evs = malloc(sizeof(struct epoll_event));
    evs->data.ptr = fd_info;
    evs->events = EPOLLET | EPOLLERR;
    if (event_type & EVE_WRITE) {
      evs->events |= EPOLLOUT;
    }
    if (event_type & EVE_READ) {
      evs->events |= EPOLLIN;
    }
    if (event_type & EVE_CLOSE) {
      evs->events |= EPOLLRDHUP;
    }
    hashmapPut(epoll_context->ep_evs, (void *)fd, evs);
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_ADD, fd, evs);
  } else {
    assert(0);
  }
  return 0;
}

static int modify(void *engine_context, int fd, int event_type) {
  zRPC_epoll_context *epoll_context = engine_context;
  struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) fd);
  if (evs != NULL) {
    evs->events = EPOLLET | EPOLLERR;
    if (event_type & EVE_WRITE) {
      evs->events |= EPOLLOUT;
    }
    if (event_type & EVE_READ) {
      evs->events |= EPOLLIN;
    }
    if (event_type & EVE_CLOSE) {
      evs->events |= EPOLLRDHUP;
    }
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, fd, evs);
  } else {
    assert(0);
  }
}

static int del(void *engine_context, int fd, void **fd_info) {
  zRPC_epoll_context *epoll_context = engine_context;
  struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) fd);
  *fd_info = evs->data.ptr;
  epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_DEL, fd, NULL);
  return 0;
}

#define MAX_EVENTS 128

static int dispatch(void *engine_context, int32_t timeout, zRPC_event_engine_result **results[], size_t *nresults) {
  zRPC_epoll_context *epoll_context = engine_context;
  struct epoll_event ep_events[MAX_EVENTS];
  int ep_rv = epoll_wait(epoll_context->ep_fd, ep_events, MAX_EVENTS, timeout);
  if (ep_rv == -1) {
    if (errno != EINTR || errno != ETIMEDOUT)
      return -1;
    return 0;
  }
  *results = calloc((size_t) ep_rv, sizeof(**results));
  for (int i = 0, j = 0; i < ep_rv; ++i) {
    void *fd_info = ep_events[i].data.ptr;
    int close = ep_events[i].events & (EPOLLRDHUP);
    int error = ep_events[i].events & (EPOLLERR | EPOLLHUP);
    int read_ev = ep_events[i].events & (EPOLLIN);
    int write_ev = ep_events[i].events & EPOLLOUT;
    int res = 0;
    if (read_ev) {
      res |= EVE_READ;
    }
    if (write_ev) {
      res |= EVE_WRITE;
    }
    if (error) {
      res |= EVE_ERROR;
    }
    if (close) {
      res |= EVE_CLOSE;
    }
    if (res == 0) {
      continue;
    }
    (*results)[i] = malloc(sizeof(zRPC_event_engine_result));
    (*results)[i]->event_type = res;
    (*results)[i]->fd = ep_events->data.fd;
    (*results)[i]->fd_info = fd_info;
  }
  *nresults = (size_t) ep_rv;
  return 0;
}