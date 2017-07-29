//
// Created by zhswo on 2016/11/22.
//
#include <netdb.h>
#include <malloc.h>
#include "server_engine.h"
#include "support/socket_utils.h"

void *initialize(zRPC_scheduler *scheduler);

void setup(void *engine_context, zRPC_pipe *pipe, const zRPC_inetaddr *addr);

int32_t start(void *engine_context);

int32_t stop(void *engine_context);

void release(void *engine_context);

const zRPC_server_engine_vtable tcp_server_engine_vtable = {
  "tcp_server",
  initialize,
  setup,
  start,
  stop,
  release
};

static int check_socket(void) {
  int has_so_reuse_port = 0;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s >= 0) {
    has_so_reuse_port = zRPC_set_socket_reuse_port(s, 1);
    close(s);
  }
  return has_so_reuse_port;
}

typedef struct zRPC_listener {
  struct zRPC_tcp_server *server;
  zRPC_channel *channel;
  struct zRPC_listener *next;
  struct zRPC_listener *sibling;
  zRPC_pipe *pipe;
  int fd;
  zRPC_inetaddr address;
  int is_sibling;
  /* Callback for accept*/
  void *accept_cb_arg;

  void *(*accept_cb)(void *);
} zRPC_listener;

typedef struct zRPC_tcp_server {
  zRPC_scheduler *scheduler;
  zRPC_listener *listener_head;
  zRPC_listener *listener_tail;
  unsigned int listeners;
}zRPC_tcp_server;

void *initialize(zRPC_scheduler *scheduler) {
  zRPC_tcp_server *tcp_server = (zRPC_tcp_server *) malloc(sizeof(zRPC_tcp_server));
  tcp_server->listener_head = NULL;
  tcp_server->listener_tail = NULL;
  tcp_server->scheduler = scheduler;
  return tcp_server;
}

static int prepare_listen_socket(int fd, const zRPC_inetaddr *address) {
  zRPC_inetaddr inetaddr_temp;
  int err = 0;
  int so_reuse_port = check_socket();
  if (so_reuse_port) {
    err = zRPC_set_socket_reuse_port(fd, 1);
    if (err != 0)
      goto error;
  }

  err = zRPC_set_socket_nonblocking(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_cloexec(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_low_latency(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_reuse_address(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_no_sigpipe_if(fd);
  if (err != 0)
    goto error;

  if (bind(fd, (struct sockaddr *) address->addr, (socklen_t) address->len) < 0) {
    err = -1;
    goto error;
  }

  if (listen(fd, 100) < 0) {
    err = -1;
    goto error;
  }

  inetaddr_temp.len = sizeof(struct sockaddr_storage);

  if (getsockname(fd, (struct sockaddr *) inetaddr_temp.addr,
                  (socklen_t *) &inetaddr_temp.len) < 0) {
    err = -1;
    goto error;
  }

  return 0;

  error:
  if (fd >= 0) {
    close(fd);
  }
  return err;
}

struct child_thread_param {
  int new_fd;
  zRPC_pipe *pipe;
};

static int child_io_thread(void *arg) {
  struct child_thread_param *param = arg;
  int new_fd = param->new_fd;
  zRPC_pipe *pipe = param->pipe;
  free(param);

  zRPC_scheduler *scheduler = zRPC_scheduler_create();
  zRPC_channel *channel;
  zRPC_channel_create(&channel, pipe, new_fd, scheduler);
  zRPC_scheduler_run(scheduler);
  return 0;
}

static void _accept_filter_on_read(zRPC_filter *filter, struct zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
  zRPC_listener *listener = zRPC_filter_get_custom_data(filter);
  zRPC_tcp_server *tcp_server = listener->server;
  int err;

  /* loop until accept4 returns EAGAIN, and then re-arm notification */
  for (;;) {
    zRPC_inetaddr address;
    char *addr_str;
    address.len = sizeof(struct sockaddr_storage);
    int new_fd = zRPC_accept4(listener->fd, &address, 1, 1);
    if (new_fd < 0) {
      switch (errno) {
        case EINTR:
          continue;
        case EAGAIN:
          return;
        default:
          return;
      }
    }

    zRPC_set_socket_no_sigpipe_if(new_fd);
    addr_str = zRPC_inetaddr_to_uri(&address);
    free(addr_str);

    struct child_thread_param *param = malloc(sizeof(struct child_thread_param));
    param->new_fd = new_fd;
    param->pipe = listener->pipe;
    zRPC_thread_id thread_id;
    zRPC_thread_create(&thread_id, child_io_thread, param, ZRPC_THREAD_FLAG_JOINABLE);
  }
}

static void _accept_filter_on_write(zRPC_filter *filter, struct zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
  zRPC_listener *listener = zRPC_filter_get_custom_data(filter);

}

static void _accept_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
  zRPC_listener *listener = zRPC_filter_get_custom_data(filter);
}

static void _accept_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
  zRPC_listener *listener = zRPC_filter_get_custom_data(filter);
}

static zRPC_filter *accept_filter_create(void *factory_custom) {
  zRPC_filter *filter;
  struct zRPC_listener *param = factory_custom;
  zRPC_filter_create(&filter, param);

  zRPC_filter_set_on_active_callback(filter, _accept_filter_on_active);
  zRPC_filter_set_on_read_callback(filter, _accept_filter_on_read);
  zRPC_filter_set_on_write_callback(filter, _accept_filter_on_write);
  zRPC_filter_set_on_inactive_callback(filter, _accept_filter_on_inactive);
  return filter;
}

zRPC_filter_factory *accept_filter_factory(zRPC_listener *listener) {
  return zRPC_filter_factory_create(accept_filter_create, listener);
}

static int add_listener_to_server(zRPC_tcp_server *server, int fd, zRPC_pipe *pipe, const zRPC_inetaddr *address, zRPC_listener **out) {
  zRPC_listener *listener = (zRPC_listener *) malloc(sizeof(zRPC_listener));
  listener->server = server;
  listener->address = *address;
  listener->next = NULL;
  listener->pipe = pipe;
  listener->fd = fd;
  prepare_listen_socket(fd, &listener->address);
  zRPC_pipe *accept_pipe;
  zRPC_pipe_create(&accept_pipe);
  zRPC_pipe_add_filter(accept_pipe, accept_filter_factory(listener));
  zRPC_channel_create(&listener->channel, accept_pipe, listener->fd, server->scheduler);
  if (server->listener_head == NULL) {
    server->listener_head = listener;
  } else {
    server->listener_tail->next = listener;
  }
  server->listener_tail = listener;
  *out = listener;
  return 0;
}

void setup(void *engine_context, zRPC_pipe *pipe, const zRPC_inetaddr *addr) {
  zRPC_tcp_server *server = engine_context;
  zRPC_socket_mode smode;
  int fd;
  int errs[2];

  if (zRPC_inetaddr_get_port(addr) == 0) {
    //TODO: report error
  }

  zRPC_listener *listener = NULL, *listener2 = NULL;
  zRPC_inetaddr wild_addr_v4, wild_addr_v6;
  if (zRPC_inetaddr_is_wildcard(addr)) {
    zRPC_inetaddr_make_wildcards(addr, &wild_addr_v4, &wild_addr_v6);

    /* Try listening on IPv6 first. */
    addr = &wild_addr_v6;
    errs[0] = zRPC_create_socket(addr, SOCK_STREAM, 0, &smode, &fd);
    if (errs[0] == 0) {
      errs[0] = add_listener_to_server(server, fd, pipe, addr, &listener);
      if (fd >= 0 && smode == ZRPC_SOCKET_IPV4_V6) {
        return;
      }
    }
    addr = &wild_addr_v4;
  }

  zRPC_inetaddr address_v4;
  errs[1] = zRPC_create_socket(addr, SOCK_STREAM, 0, &smode, &fd);
  if (errs[1] == 0) {
    if (smode == ZRPC_SOCKET_IPV4 && zRPC_inetaddr_is_v4_mapped(addr)) {
      zRPC_inetaddr_to_v4_mapped(addr, &address_v4);
      addr = &address_v4;
    }
    listener2 = listener;
    errs[1] = add_listener_to_server(server, fd, pipe, addr, &listener);
    if (listener2 != NULL && listener != NULL) {
      listener2->sibling = listener;
      listener->is_sibling = 1;
    }
  }
}

int32_t start(void *engine_context) {
  zRPC_tcp_server *server = engine_context;
  zRPC_listener *listener;
  listener = server->listener_head;
  while (listener != NULL) {
    listener = listener->next;
    zRPC_scheduler_register_source(listener->server->scheduler, &listener->channel->source);
  }
}

int32_t stop(void *engine_context) {
  zRPC_tcp_server *server = engine_context;
  zRPC_listener *listener;
  listener = server->listener_head;
  while (listener != NULL) {
    listener = listener->next;
    zRPC_scheduler_unregister_source(listener->server->scheduler, &listener->channel->source);
  }
}

void release(void *engine_context) {
  free(engine_context);
}