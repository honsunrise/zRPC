//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_EVENT_ENGINE_H
#define ZRPC_EVENT_ENGINE_H

#include "event.h"
#include "zRPC/support/time.h"

struct zRPC_context;

typedef struct zRPC_event_engine_vtable {
    const char *name;

    void *(*initialize)(struct zRPC_context *);

    int (*add)(struct zRPC_context *, zRPC_event *);

    int (*del)(struct zRPC_context *, zRPC_event *);

    int (*dispatch)(struct zRPC_context *, zRPC_timespec *ts);

    void (*uninitialize)(struct zRPC_context *);

    size_t context_size;
} zRPC_event_engine_vtable;

#endif //ZRPC_EVENT_ENGINE_H
