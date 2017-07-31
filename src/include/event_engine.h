//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_EVENT_ENGINE_H
#define ZRPC_EVENT_ENGINE_H
#include <stdlib.h>

typedef enum EVE_EVENT_TYPE {
  EVE_READ = 0x01,
  EVE_WRITE = 0x02,
  EVE_CLOSE = 0x04,
  EVE_ERROR = 0x08,
} zRPC_EVE_EVENT_TYPE;

typedef struct zRPC_event_engine_result {
  int fd;
  void *fd_info;
  int event_type;
} zRPC_event_engine_result;

typedef struct zRPC_event_engine_vtable {
  const char *name;

  void *(*initialize)();

  int (*set)(void *engine_context, int fd, void *fd_info, int event_type);

  int (*del)(void *engine_context, int fd, void **fd_info);

  int (*dispatch)(void *engine_context, int32_t timeout, zRPC_event_engine_result **results[], size_t *nresults);

  void (*release)(void *engine_context);
} zRPC_event_engine_vtable;

inline void zRPC_event_engine_release_result(zRPC_event_engine_result *results[], size_t nresults) {
  for (int i = 0; i < nresults; ++i) {
    free(results[i]);
  }
  free(results);
}
#endif //ZRPC_EVENT_ENGINE_H
