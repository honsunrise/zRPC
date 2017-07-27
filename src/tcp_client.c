//
// Created by zhsyourai on 11/28/16.
//

#include "client_engine.h"
#include "zRPC/support/socket_utils.h"

static int prepare_socket(const zRPC_inetaddr *addr, int fd) {
  int err = 0;

  err = zRPC_set_socket_nonblocking(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_cloexec(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_low_latency(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_no_sigpipe_if(fd);
  if (err != 0)
    goto error;
  goto done;

  error:
  if (fd >= 0) {
    close(fd);
  }
  done:
  return err;
}

void *initialize(zRPC_scheduler *scheduler, zRPC_pipe *pipe, const zRPC_inetaddr *addr);

zRPC_channel *start(void *engine_context);

int32_t stop(void *engine_context);

void release(void *engine_context);

const zRPC_client_engine_vtable tcp_client_engine_vtable = {
    "tcp_client",
    initialize,
    start,
    stop,
    release
};

typedef struct _tcp_client {
  zRPC_scheduler *scheduler;
  zRPC_channel *channel;
  zRPC_pipe *pipe;
  zRPC_inetaddr addr;
} _tcp_client;

void *initialize(zRPC_scheduler *scheduler, zRPC_pipe *pipe, const zRPC_inetaddr *addr) {
  _tcp_client *tcp_client = (_tcp_client *) malloc(sizeof(_tcp_client));
  tcp_client->pipe = pipe;
  tcp_client->addr = *addr;
  tcp_client->scheduler = scheduler;
  return tcp_client;
}

zRPC_channel *start(void *engine_context) {
  _tcp_client *tcp_client = engine_context;
  int origin_fd;
  zRPC_socket_mode s_mode;
  int err;
  zRPC_inetaddr addr6_v4mapped;
  zRPC_inetaddr addr4_copy;
  char *addr_str;
  int error;

  if (zRPC_inetaddr_is_v4_mapped(&tcp_client->addr)) {
    zRPC_inetaddr_to_v4_mapped(&tcp_client->addr, &addr6_v4mapped);
    tcp_client->addr = addr6_v4mapped;
  }

  error = zRPC_create_socket(&tcp_client->addr, SOCK_STREAM, 0, &s_mode, &origin_fd);
  if (error != 0) {
    return NULL;
  }
  if (s_mode == ZRPC_SOCKET_IPV4) {
    if (zRPC_inetaddr_is_v4_mapped(&tcp_client->addr)) {
      zRPC_inetaddr_to_v4_mapped(&tcp_client->addr, &addr4_copy);
      tcp_client->addr = addr4_copy;
    }
  }
  if ((error = prepare_socket(&tcp_client->addr, origin_fd)) != 0) {
    return NULL;
  }

  do {
    err = connect(origin_fd, (const struct sockaddr *) tcp_client->addr.addr, (socklen_t) tcp_client->addr.len);
  } while (err < 0 && errno == EINTR);

  addr_str = zRPC_inetaddr_to_uri(&tcp_client->addr);

  zRPC_channel *channel;
  zRPC_channel_create(&channel, tcp_client->pipe, origin_fd, tcp_client->scheduler);
  tcp_client->channel = channel;
  free(addr_str);
  return channel;
}

int32_t stop(void *engine_context) {
  _tcp_client *tcp_client = engine_context;
  zRPC_channel_destroy(tcp_client->channel);
}

void release(void *engine_context) {
  free(engine_context);
}
