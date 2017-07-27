//
// Created by zhsyourai on 11/28/16.
//

#ifndef ZRPC_CLIENT_H
#define ZRPC_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduling.h"
#include "channel.h"
#include "event_engine.h"

typedef struct zRPC_client zRPC_client;

zRPC_client *zRPC_client_create(zRPC_scheduler *scheduler, zRPC_pipe *pipe);

void zRPC_client_connect(zRPC_client *client, const char *hostname);

zRPC_channel *zRPC_client_get_channel(zRPC_client *client);
#ifdef __cplusplus
}
#endif
#endif //ZRPC_CLIENT_H
