//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_EVENT_ENGINE_H
#define ZRPC_EVENT_ENGINE_H

#include "event.h"

typedef struct zRPC_event_engine_vtable {
  const char *name;

  void *(*initialize)();

  int (*add)(void *engine_context, zRPC_event *event);

  int (*del)(void *engine_context, zRPC_event *event);

  int (*dispatch)(void *engine_context, uint32_t timeout, zRPC_event *events[], size_t *nevents);

  void (*release)(void *engine_context);

  size_t engine_context_size;
} zRPC_event_engine_vtable;

#endif //ZRPC_EVENT_ENGINE_H
