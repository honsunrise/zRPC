//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_CONTEXT_H
#define ZRPC_CONTEXT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "support/thread.h"
#include "ds/list.h"
#include "support/lock.h"
#include "support/runnable.h"
#include "timer.h"
#include "event_engine.h"
#include "timer_engine.h"
#include "notify.h"
#include "zRPC/resolver/resolver.h"

typedef struct zRPC_scheduler {
  zRPC_mutex global_mutex;
  zRPC_cond global_cond;
  int exit_loop;
  void *event_engine_context;
  const zRPC_event_engine_vtable *event_engine;
  void *timer_engine_context;
  const zRPC_timer_engine_vtable *timer_engine;
  zRPC_list_head source_list_head;
  int running_loop;
  zRPC_thread_id owner_thread_id;
  zRPC_queue *event_queue;
  zRPC_notify *notify;
} zRPC_scheduler;

zRPC_scheduler *zRPC_scheduler_create();

int zRPC_scheduler_register_source(zRPC_scheduler *scheduler, zRPC_event_source *source);

int zRPC_scheduler_unregister_source(zRPC_scheduler *scheduler, zRPC_event_source *source);

void zRPC_scheduler_outer_event(zRPC_scheduler *scheduler, zRPC_event event);

void zRPC_scheduler_notify(zRPC_scheduler *scheduler);

void zRPC_scheduler_destroy(zRPC_scheduler *scheduler);

void zRPC_scheduler_run(zRPC_scheduler *scheduler);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CONTEXT_H
