//
// Created by zhsyourai on 11/28/16.
//

#ifndef ZRPC_CLIENT_H
#define ZRPC_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "context.h"
#include "channel.h"
#include "event_engine.h"

typedef struct zRPC_client zRPC_client;

zRPC_client *zRPC_client_create(zRPC_context *context, const char *hostname, zRPC_pipe *pipe);

zRPC_pipe *zRPC_client_get_pipe(zRPC_client *client);

void zRPC_client_connect(zRPC_client *client);

zRPC_channel *zRPC_client_get_channel(zRPC_client *client);

zRPC_context *zRPC_client_get_context(zRPC_client *client);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CLIENT_H
