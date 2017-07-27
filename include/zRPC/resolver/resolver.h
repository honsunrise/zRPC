//
// Created by zhsyourai on 1/24/17.
//

#ifndef ZRPC_RESOLVER_H
#define ZRPC_RESOLVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/timer.h"
#include "zRPC/support/inetaddr_utils.h"

typedef struct zRPC_resolved zRPC_resolved;

typedef void (*zRPC_resolver_complete_callback)(void *custom_arg, zRPC_resolved *resolved);

struct zRPC_resolved {
    const char *resolve_name;
    zRPC_inetaddres inetaddres;
    zRPC_resolver_complete_callback callback;
    void *custom_arg;
    zRPC_timer *retry_timer;
};

typedef struct zRPC_resolved_holder {
    zRPC_resolved *resolveds;
    size_t resolved_cap;
    size_t resolved_count;
} zRPC_resolved_holder;

void zRPC_resolver_address(struct zRPC_scheduler *context, const char *name, zRPC_resolver_complete_callback callback,
                           void *custom_arg);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_RESOLVER_H
