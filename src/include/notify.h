//
// Created by zhsyourai on 7/24/17.
//

#ifndef ZRPC_NOTIFY_H
#define ZRPC_NOTIFY_H
#include "source.h"
#include "zRPC/scheduling.h"

typedef struct zRPC_notify {
  zRPC_event_source source;
  int is_notify_pending;
  int notify_fd[2];
} zRPC_notify;

void zRPC_notify_create(zRPC_notify **out, zRPC_scheduler *scheduler);

void zRPC_notify_destroy(zRPC_notify *notify);

void zRPC_notify_write(zRPC_notify *notify);

#endif //ZRPC_NOTIFY_H
