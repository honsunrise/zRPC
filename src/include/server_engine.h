//
// Created by zhsyourai on 7/27/17.
//

#ifndef ZRPC_SERVER_ENGINE_H
#define ZRPC_SERVER_ENGINE_H
#include "zRPC/scheduling.h"
#include "channel.h"
typedef struct zRPC_server_engine_vtable {
  const char *name;

  void *(*initialize)(zRPC_scheduler *scheduler);

  void (*setup)(void *engine_context, zRPC_pipe *pipe, const zRPC_inetaddr *addr);

  int32_t (*start)(void *engine_context);

  int32_t (*stop)(void *engine_context);

  void (*release)(void *engine_context);
} zRPC_server_engine_vtable;
#endif //ZRPC_SERVER_ENGINE_H
