//
// Created by zcodes on 2016/11/22.
//

#ifndef ZRPC_TCP_SERVER_H
#define ZRPC_TCP_SERVE

#include <stdio.h>
#include "server_context.h"
#include "zRPC/support/inetaddr_utils.h"
#include "zRPC/support/socket_utils.h"
#include "event.h"
#include "channel.h"
#include "server.h"

typedef struct zRPC_tcp_server zRPC_tcp_server;

static int has_so_reuse_port = 0;

static void check_socket(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s >= 0) {
        has_so_reuse_port = zRPC_set_socket_reuse_port(s, 1);
        close(s);
    }
}

zRPC_tcp_server *zRPC_tcp_server_create(zRPC_server *server);

void zRPC_tcp_server_add_listener(zRPC_tcp_server *server, zRPC_inetaddr *address);

void zRPC_tcp_server_start(zRPC_tcp_server *server);

#endif //ZRPC_TCP_SERVER_H
