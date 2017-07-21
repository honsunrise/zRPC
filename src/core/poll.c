//
// Created by zhswo on 2016/11/23.
//

#include <memory.h>
#include <sys/poll.h>
#include "zRPC/scheduling.h"
#include "ds/hashmap.h"

static void *initialize();

static int add(void *engine_context, int fd, void *fd_info, int event_type);

static int del(void *engine_context, int fd);

static int dispatch(void *engine_context, int32_t timeout, zRPC_event_engine_result *results[], size_t *nresults);

static void release(void *engine_context);

static int int_hash_function(void *key) {
  return (int) (key);
}

static int equals(void *keyA, void *keyB) {
  return keyA == keyB;
}

typedef struct zRPC_poll_context {
  struct pollfd *fds;
  unsigned int nfds;
  unsigned int fds_cap;
  zRPC_hashmap *fd_map;
} zRPC_poll_context;

typedef struct hashmap_entry {
  int poll_fds_index;
  int fd;
  void *fd_info;
} hashmap_entry;

const zRPC_event_engine_vtable poll_event_engine_vtable = {
    "poll",
    initialize,
    add,
    del,
    dispatch,
    release
};

static void *initialize() {
  zRPC_poll_context *poll_context = malloc(sizeof(zRPC_poll_context));
  poll_context->nfds = 0;
  poll_context->fds_cap = 32;
  poll_context->fds = calloc(poll_context->fds_cap, sizeof(struct pollfd));
  poll_context->fd_map = hashmapCreate(1024, int_hash_function, equals);
  return poll_context;
}

static void release(void *engine_context) {
  if (engine_context) {
    hashmapFree(((zRPC_poll_context *) engine_context)->fd_map);
    free(((zRPC_poll_context *) engine_context)->fds);
    free(engine_context);
  }
}

static int add(void *engine_context, int fd, void *fd_info, int event_type) {
  zRPC_poll_context *poll_context = engine_context;
  short care = POLLERR | POLLHUP | POLLNVAL;
  if (event_type & EVE_WRITE) {
    care |= POLLOUT;
  }
  if (event_type & EVE_READ) {
    care |= POLLIN;
  }
  hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *) fd);
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
    entry->fd = fd;
    entry->poll_fds_index = poll_context->nfds;
    entry->fd_info = fd_info;
    hashmapPut(poll_context->fd_map, (void *) fd, entry);
    poll_context->fds[poll_context->nfds].fd = fd;
    poll_context->fds[poll_context->nfds++].events = care;
  } else {
    poll_context->fds[entry->poll_fds_index].events |= care;
  }
  return 0;
}

static int del(void *engine_context, int fd) {
  zRPC_poll_context *poll_context = engine_context;
  hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *)fd);
  struct pollfd *move = &poll_context->fds[poll_context->nfds - 1];
  hashmap_entry *move_entry = hashmapGet(poll_context->fd_map, (void *) move->fd);
  assert(move_entry != NULL);
  move_entry->poll_fds_index = entry->poll_fds_index;
  memcpy(&poll_context->fds[entry->poll_fds_index], &poll_context->fds[--poll_context->nfds], sizeof(struct pollfd));
  hashmapRemove(poll_context->fd_map, (void *) fd);
  free(entry);
  return 0;
}

static int dispatch(void *engine_context, int32_t timeout, zRPC_event_engine_result *results[], size_t *nresults) {
  zRPC_poll_context *poll_context = engine_context;

  struct pollfd *event_set;
  nfds_t nfds = poll_context->nfds;
  event_set = poll_context->fds;
  int p_rv = poll(event_set, nfds, timeout);
  if (p_rv == -1) {
    if (errno != EINTR || errno != ETIMEDOUT)
      return -1;
    return 0;
  }

  int i = 0;
  for (int j = 0; j < nfds; j++) {
    int happen = event_set[j].revents;
    if (!happen)
      continue;
    hashmap_entry *entry = hashmapGet(poll_context->fd_map, (void *) event_set[j].fd);
    assert(entry != NULL);
    void *fd_info = entry->fd_info;
    int error = happen & (POLLHUP | POLLERR | POLLNVAL);
    int read_ev = happen & POLLIN;
    int write_ev = happen & POLLOUT;
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
    if (res == 0) {
      continue;
    }
    results[i] = malloc(sizeof(zRPC_event_engine_result));
    results[i]->event_type = res;
    results[i]->fd = entry->fd;
    results[i]->fd_info = fd_info;
  }
  return 0;
}