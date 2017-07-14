//
// Created by zhswo on 2016/11/23.
//

#include <memory.h>
#include <sys/poll.h>
#include "zRPC/context.h"
#include "zRPC/ds/hashmap.h"

typedef struct zRPC_poll_context {
  zRPC_context *context;
  struct pollfd *fds;
  unsigned int nfds;
  unsigned int fds_cap;
  zRPC_hashmap *fd_map;
} zRPC_poll_context;

static void *initialize(zRPC_context *);

static int add(zRPC_context *, zRPC_event *);

static int del(zRPC_context *, zRPC_event *);

static int dispatch(zRPC_context *, zRPC_timespec *ts);

static void release(zRPC_context *);

static int int_hash_function(void *key) {
  return (int) (key);
}

static int equals(void *keyA, void *keyB) {
  return keyA == keyB;
}

typedef struct hashmap_entry {
  int pollfds_index;
  zRPC_sample_fd *fd;
} hashmap_entry;

const zRPC_event_engine_vtable poll_event_engine_vtable = {
    "poll",
    initialize,
    add,
    del,
    dispatch,
    release,
    sizeof(zRPC_poll_context)
};

static void *initialize(zRPC_context *context) {
  zRPC_poll_context *poll_context = malloc(sizeof(zRPC_poll_context));
  poll_context->context = context;
  poll_context->nfds = 0;
  poll_context->fds_cap = 32;
  poll_context->fds = calloc(poll_context->fds_cap, sizeof(struct pollfd));
  poll_context->fd_map = hashmapCreate(1024, int_hash_function, equals);
  return poll_context;
}

static void release(zRPC_context *context) {
  if (context->event_engine_context) {
    hashmapFree(((zRPC_poll_context *) context->event_engine_context)->fd_map);
    free(((zRPC_poll_context *) context->event_engine_context)->fds);
    free(context->event_engine_context);
  }
}

static int add(zRPC_context *context, zRPC_event *event) {
  zRPC_poll_context *poll_context = context->event_engine_context;
  if (!(event->event_status & EVS_INIT)) {
    return -1;
  }
  short care = POLLERR | POLLHUP | POLLNVAL;
  if (event->event_type & EV_WRITE) {
    care |= POLLOUT;
  }
  if (event->event_type & EV_READ) {
    care |= POLLIN;
  }
  hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *) zRPC_fd_origin(event->fd));
  if (entry == NULL) {
    if (poll_context->nfds >= poll_context->fds_cap) {
      unsigned int new_size = poll_context->fds_cap * 2;
      struct pollfd *new_fds = realloc(poll_context->fds, new_size * sizeof(zRPC_poll_context));
      if (new_fds == NULL) {
        return -1;
      }
      poll_context->fds = new_fds;
      poll_context->fds_cap = new_size;
    }
    entry = malloc(sizeof(hashmap_entry));
    entry->fd = event->fd;
    entry->pollfds_index = poll_context->nfds;
    hashmapPut(poll_context->fd_map, (void *) zRPC_fd_origin(event->fd), entry);
    poll_context->fds[poll_context->nfds].fd = zRPC_fd_origin(event->fd);
    poll_context->fds[poll_context->nfds++].events = care;
  } else {
    poll_context->fds[entry->pollfds_index].events |= care;
  }

  event->event_status = EVS_REGISTER;
  return 0;
}

static int del(zRPC_context *context, zRPC_event *event) {
  zRPC_poll_context *poll_context = context->event_engine_context;
  if (!(event->event_status & EVS_REGISTER)) {
    return -1;
  }
  hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *) zRPC_fd_origin(event->fd));
  struct pollfd *pfd = &poll_context->fds[entry->pollfds_index];
  if (event->event_type & EV_READ)
    pfd->events &= ~POLLIN;
  if (event->event_type & EV_WRITE)
    pfd->events &= ~POLLOUT;
  if (pfd->events)
    return 0;
  struct pollfd *move = &poll_context->fds[poll_context->nfds - 1];
  hashmap_entry *move_entry = hashmapGet(poll_context->fd_map, (void *) move->fd);
  assert(move_entry != NULL);
  move_entry->pollfds_index = entry->pollfds_index;
  memcpy(&poll_context->fds[entry->pollfds_index], &poll_context->fds[--poll_context->nfds], sizeof(struct pollfd));
  hashmapRemove(poll_context->fd_map, (void *) zRPC_fd_origin(event->fd));
  free(entry);
  return 0;
}

static int dispatch(zRPC_context *context, zRPC_timespec *ts) {
  zRPC_poll_context *poll_context = context->event_engine_context;

  struct pollfd *event_set;
  nfds_t nfds = poll_context->nfds;
  event_set = poll_context->fds;
  int p_rv = poll(event_set, nfds, zRPC_time_to_millis(*ts));
  if (p_rv == -1) {
    if (errno != EINTR || errno != ETIMEDOUT)
      return -1;
    return 0;
  }

  for (int j = 0; j < nfds; j++) {
    int happen = event_set[j].revents;
    if (!happen)
      continue;
    hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *) event_set[j].fd);
    assert(entry != NULL);
    int cancel = happen & (POLLHUP | POLLERR | POLLNVAL);
    int read_ev = happen & POLLIN;
    int write_ev = happen & POLLOUT;
    int res = 0;
    if (read_ev) {
      res |= EV_READ;
    }
    if (write_ev) {
      res |= EV_WRITE;
    }
    if (cancel) {
      res |= EV_ERROR;
    }
    if (res == 0) {
      continue;
    }
    zRPC_context_fd_event_happen(context, entry->fd, res);
  }
  return (0);
}