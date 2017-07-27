//
// Created by zhsyourai on 7/27/17.
//

#ifndef ZRPC_CLIENT_ENGINE_H
#define ZRPC_CLIENT_ENGINE_H
#include "zRPC/scheduling.h"
#include "channel.h"
typedef struct zRPC_client_engine_vtable {
  const char *name;

  void *(*initialize)(zRPC_scheduler *scheduler, zRPC_pipe *pipe, const zRPC_inetaddr *addr);

  zRPC_channel* (*start)(void *engine_context);

  int32_t (*stop)(void *engine_context);

  void (*release)(void *engine_context);
} zRPC_client_engine_vtable;
#endif //ZRPC_CLIENT_ENGINE_H
