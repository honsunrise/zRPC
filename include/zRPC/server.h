//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_SERVER_H
#define ZRPC_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "channel.h"
#include "event.h"
#include "event_engine.h"

typedef struct zRPC_server zRPC_server;

zRPC_server *zRPC_server_create(struct zRPC_scheduler *context, zRPC_pipe *pipe);

void zRPC_server_start(zRPC_server *server, const char *addr);

void zRPC_server_stop(zRPC_server *server);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_SERVER_H
