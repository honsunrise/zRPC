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

  int (*add)(void *engine_context, zRPC_timer_task *task);

  int (*del)(void *engine_context, zRPC_timer_task *task);

  int32_t (*dispatch)(void *engine_context, zRPC_timer_task **results[], size_t *nresults);

  void (*release)(void *engine_context);
} zRPC_timer_engine_vtable;

void zRPC_timer_engine_release_result(zRPC_timer_task **results, size_t nresults);

#endif //ZRPC_TIMER_ENGINE_H
