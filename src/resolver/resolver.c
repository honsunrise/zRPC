//
// Created by zhsyourai on 1/24/17.
//

#include <arpa/inet.h>
#include <netdb.h>
#include "support/socket_utils.h"
#include "support/string_utils.h"
#include "zRPC/scheduling.h"

static char *svc[][2] = {{"http", "80"},
                         {"https", "443"}};

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
  zRPC_scheduler *scheduler;
  zRPC_resolved *resolved;
} zRPC_resolved_param;

static void complete_runnable(zRPC_resolved *resolved) {
  resolved->callback(resolved->custom_arg, resolved);
  free(resolved->inetaddres.addrs);
}

static void _on_timer_retry(zRPC_timespec deadline, void *param) {
  zRPC_resolved_param *resolved_param = param;
  zRPC_resolved *resolved = resolved_param->resolved;

  blocking_resolve_address_impl(resolved->resolve_name, &resolved->inetaddres);

  if (&resolved->inetaddres.naddrs != 0) {
    free(resolved_param);
    complete_runnable(resolved);
  } else {
    zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
    deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(10, zRPC_CLOCK_MONOTONIC));
    resolved->retry_timer = zRPC_timer_create(resolved_param->scheduler);
    zRPC_timer_deadline(resolved->retry_timer, deadline, _on_timer_retry, param);
  }
}

void zRPC_resolver_address(zRPC_scheduler *scheduler, const char *name, zRPC_resolver_complete_callback callback,
                           void *custom_arg) {
  zRPC_resolved *resolved = malloc(sizeof(zRPC_resolved));
  resolved->resolve_name = zRPC_str_dup(name);
  resolved->callback = callback;
  resolved->custom_arg = custom_arg;
  zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
  deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(0, zRPC_CLOCK_MONOTONIC));
  zRPC_resolved_param *param = malloc(sizeof(zRPC_resolved_param));
  param->resolved = resolved;
  param->scheduler = scheduler;
  resolved->retry_timer = zRPC_timer_create(scheduler);
  zRPC_timer_deadline(resolved->retry_timer, deadline, _on_timer_retry, param);
}