//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_CONTEXT_H
#define ZRPC_CONTEXT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/support/thread.h"
#include "zRPC/ds/list.h"
#include "zRPC/support/lock.h"
#include "zRPC/support/runnable.h"
#include "zRPC/support/timer.h"
#include "event_engine.h"
#include "zRPC/resolver/resolver.h"

typedef struct zRPC_context {
    zRPC_mutex global_mutex;
    zRPC_cond global_cond;
    int global_threads;
    zRPC_list_head event_register;
    zRPC_list_head event_active;
    size_t event_active_count;
    zRPC_list_head event_pending;
    zRPC_list_head event_remove;
    void *event_engine_context;
    const zRPC_event_engine_vtable *event_engine;
    zRPC_timer_holder *timer_holder;
    zRPC_fd *notify_fd[2];
    zRPC_event *notify_event;
    zRPC_runnable *notify_runnable;
    int is_notify_pending;
    int running_loop;
    zRPC_timespec ts_cache;

    zRPC_resolved_holder *resolved_holder;
    zRPC_thread_id owner_thread_id;
} zRPC_context;

zRPC_context *zRPC_context_create();

int zRPC_context_register_event(zRPC_context *context, zRPC_event *event);

int zRPC_context_unregister_event(zRPC_context *context, zRPC_event *event);

size_t zRPC_context_unregister_event_fd(zRPC_context *context, zRPC_fd *fd);

void zRPC_context_fd_event_happen(zRPC_context *context, zRPC_fd *fd, int res);

void zRPC_context_timer_event_happen(zRPC_context *context, zRPC_timer *timer, int res);

void zRPC_context_notify(zRPC_context *context);

void zRPC_context_destroy(zRPC_context *context);

void zRPC_context_dispatch(zRPC_context *context);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CONTEXT_H
