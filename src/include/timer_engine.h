//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_TIMER_ENGINE_H
#define ZRPC_TIMER_ENGINE_H

#include <stdint.h>
#include "zRPC/timer.h"

typedef struct zRPC_timer_engine_vtable {
  const char *name;

  void *(*initialize)();

  int (*add)(void *engine_context, zRPC_timer *timer);

  int (*del)(void *engine_context, zRPC_timer *timer);

  int32_t (*dispatch)(void *engine_context, zRPC_timer **results[], size_t *nresults);

  void (*release)(void *engine_context);
} zRPC_timer_engine_vtable;

inline void zRPC_timer_engine_release_result(zRPC_timer **results, size_t nresults) {
  for (int i = 0; i < nresults; ++i) {
    free(results[i]);
  }
  free(results);
}
#endif //ZRPC_TIMER_ENGINE_H
