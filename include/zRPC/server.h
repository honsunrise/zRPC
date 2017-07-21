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

zRPC_server *zRPC_server_create(struct zRPC_scheduler *context, const char *addr, zRPC_pipe *pipe);

void zRPC_server_start(zRPC_server *server);

void zRPC_server_add_channel(zRPC_server *server, zRPC_channel *channel);

void zRPC_server_get_channels(zRPC_server *server, zRPC_channel ***out, unsigned int *count);

zRPC_pipe *zRPC_server_get_pipe(zRPC_server *server);

struct zRPC_scheduler *zRPC_server_get_context(zRPC_server *server);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_SERVER_H
