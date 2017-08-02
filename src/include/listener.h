//
// Created by zhsyourai on 7/24/17.
//

#ifndef ZRPC_ACCEPTER_H
#define ZRPC_ACCEPTER_H
#include "source.h"
#include "channel.h"
#include "support/socket_utils.h"

struct zRPC_listener;

typedef void (*accepter_listener_callback)(int new_fd, void *param);

typedef struct zRPC_listener {
  zRPC_event_source source;
  int fd;
  void *callback_arg;
  accepter_listener_callback callback;
  zRPC_inetaddr address;
} zRPC_listener;

void zRPC_listener_create(zRPC_listener **out,
                          zRPC_socket_mode *smode,
                          zRPC_inetaddr address,
                          accepter_listener_callback callback,
                          void *param,
                          zRPC_scheduler *scheduler);

void zRPC_listener_destroy(zRPC_listener *listener);

#endif //ZRPC_ACCEPTER_H
