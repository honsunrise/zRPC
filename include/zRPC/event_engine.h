//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_EVENT_ENGINE_H
#define ZRPC_EVENT_ENGINE_H

typedef enum EVE_EVENT_TYPE {
  EVE_READ,
  EVE_WRITE,
  EVE_CLOSE,
  EVE_ERROR,
} EVE_EVENT_TYPE;

typedef struct zRPC_event_engine_result {
  int fd;
  void *fd_info;
  EVE_EVENT_TYPE event_type;
} zRPC_event_engine_result;

typedef struct zRPC_event_engine_vtable {
  const char *name;

  void *(*initialize)();

  int (*add)(void *engine_context, int fd, void *fd_info, EVE_EVENT_TYPE event_type);

  int (*del)(void *engine_context, int fd, EVE_EVENT_TYPE event_type);

  int (*dispatch)(void *engine_context, uint32_t timeout, zRPC_event_engine_result *results[], size_t *nresults);

  void (*release)(void *engine_context);
} zRPC_event_engine_vtable;

#endif //ZRPC_EVENT_ENGINE_H
