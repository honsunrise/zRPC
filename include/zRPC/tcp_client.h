//
// Created by zhsyourai on 11/28/16.
//

#ifndef ZRPC_TCP_CLIENT_H
#define ZRPC_TCP_CLIENT_H

#include "client.h"
#include "zRPC/support/inetaddr_utils.h"

typedef struct zRPC_tcp_client zRPC_tcp_client;

zRPC_tcp_client *zRPC_tcp_client_create(zRPC_client *client);

void zRPC_tpc_client_set_addr(zRPC_tcp_client *client, zRPC_inetaddr *addr);

zRPC_channel *zRPC_tcp_client_start(zRPC_tcp_client *client);

#endif //ZRPC_TCP_CLIENT_H
