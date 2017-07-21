//
// Created by zhsyourai on 1/24/17.
//

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "zRPC/support/socket_utils.h"
#include "zRPC/support/string_utils.h"
#include "zRPC/scheduling.h"

static char *svc[][2] = {{"http",  "80"},
                         {"https", "443"}};

void zRPC_resolver_init(zRPC_scheduler *context) {
    context->resolved_holder = malloc(sizeof(zRPC_resolved_holder));
    context->resolved_holder->resolved_cap = 10;
    context->resolved_holder->resolveds = malloc(sizeof(zRPC_resolved) * context->resolved_holder->resolved_cap);
    context->resolved_holder->resolved_count = 0;
}

void zRPC_resolver_shutdown(zRPC_scheduler *context) {

}

static void blocking_resolve_address_impl(const char *name, zRPC_inetaddres *addresses) {
    struct addrinfo hints;
    struct addrinfo *result = NULL, *resp;
    char *host;
    char *port;
    int s;
    size_t i;

    /* parse name, splitting it into host and port parts */
    zRPC_split_host_port(name, &host, &port);
    if (host == NULL) {
        goto done;
    }
    if (port == NULL) {
        goto done;
    }

    /* Call getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* ipv4 or ipv6 */
    hints.ai_socktype = SOCK_STREAM; /* stream socket */
    hints.ai_flags = AI_PASSIVE;     /* for wildcard IP address */

    s = getaddrinfo(host, port, &hints, &result);

    if (s != 0) {
        for (i = 0; i < sizeof(svc) / sizeof(*svc); i++) {
            if (strcmp(port, svc[i][0]) == 0) {
                s = getaddrinfo(host, svc[i][1], &hints, &result);
                break;
            }
        }
    }

    if (s != 0) {
        goto done;
    }

    /* Success path: set addrs non-NULL, fill it in */
    addresses->naddrs = 0;
    for (resp = result; resp != NULL; resp = resp->ai_next) {
        addresses->naddrs++;
    }
    addresses->addrs = malloc(sizeof(zRPC_inetaddr) * addresses->naddrs);
    i = 0;
    for (resp = result; resp != NULL; resp = resp->ai_next) {
        memcpy(&addresses->addrs[i].addr, resp->ai_addr, resp->ai_addrlen);
        addresses->addrs[i].len = resp->ai_addrlen;
        i++;
    }
    done:
    free(host);
    free(port);
    if (result) {
        freeaddrinfo(result);
    }
}

typedef struct zRPC_resolved_param {
    zRPC_scheduler *context;
    zRPC_resolved *resolved;
} zRPC_resolved_param;


static void zRPC_on_timer_retry(zRPC_resolved_param *param) {
    zRPC_resolved *resolved = param->resolved;

    blocking_resolve_address_impl(resolved->resolve_name, &resolved->inetaddres);

    if (&resolved->inetaddres.naddrs != 0) {
        free(param);
        zRPC_runnable_run(resolved->complete_callback);
    } else {
        zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
        deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(0, zRPC_CLOCK_MONOTONIC));
        zRPC_runnable *runnable = zRPC_runnable_create((void *(*)(void *)) zRPC_on_timer_retry, param->context,
                                                       zRPC_runnable_release_callback);
        resolved->retry_timer = zRPC_timer_schedule(param->context, deadline, runnable);
    }
}

static void complete_runnable(void *arg) {
    zRPC_resolved *resolved = arg;
    resolved->callback(resolved->custom_arg, resolved);
    free(resolved->inetaddres.addrs);
}

void zRPC_resolver_address(zRPC_scheduler *context, const char *name, zRPC_resolver_complete_callback callback,
                           void *custom_arg) {
    zRPC_resolved *resolved = &context->resolved_holder->resolveds[context->resolved_holder->resolved_count];
    resolved->resolve_name = zRPC_str_dup(name);
    resolved->callback = callback;
    resolved->custom_arg = custom_arg;
    zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
    deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(0, zRPC_CLOCK_MONOTONIC));
    zRPC_resolved_param *param = malloc(sizeof(zRPC_resolved_param));
    param->resolved = resolved;
    param->context = context;
    resolved->complete_callback = zRPC_runnable_create((void *(*)(void *)) complete_runnable, resolved,
                                                       zRPC_runnable_release_callback);
    zRPC_runnable *runnable = zRPC_runnable_create((void *(*)(void *)) zRPC_on_timer_retry, param,
                                                   zRPC_runnable_release_callback);
    resolved->retry_timer = zRPC_timer_schedule(context, deadline, runnable);
    ++context->resolved_holder->resolved_count;
}