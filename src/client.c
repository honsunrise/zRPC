//
// Created by zhsyourai on 11/28/16.
//

#include "zRPC/support/socket_utils.h"
#include "zRPC/support/string_utils.h"
#include "zRPC/timer.h"
#include "zRPC/client.h"
#include "zRPC/tcp_client.h"

struct zRPC_client {
    zRPC_scheduler *context;
    zRPC_pipe *pipe;
    zRPC_channel *channel;
    zRPC_tcp_client *tcp_client;
    char *hostname;
};


zRPC_client *zRPC_client_create(zRPC_scheduler *context, const char *hostname, zRPC_pipe *pipe) {
    zRPC_client *client = (zRPC_client *) malloc(sizeof(zRPC_client));
    client->context = context;
    client->pipe = pipe;
    client->tcp_client = zRPC_tcp_client_create(client);
    client->hostname = zRPC_str_dup(hostname);
    return client;
}

static void resolver_complete_callback(void *custom_arg, zRPC_resolved *resolved) {
    zRPC_client *client = custom_arg;
    for (int i = 0; i < resolved->inetaddres.naddrs;) {
        zRPC_tpc_client_set_addr(client->tcp_client, &resolved->inetaddres.addrs[i]);
        client->channel = zRPC_tcp_client_start(client->tcp_client);
        return;
    }
}

void zRPC_client_connect(zRPC_client *client) {
    zRPC_resolver_address(client->context, client->hostname, resolver_complete_callback, client);
}

zRPC_pipe *zRPC_client_get_pipe(zRPC_client *client) {
    return client->pipe;
}

zRPC_channel *zRPC_client_get_channel(zRPC_client *client) {
    return client->channel;
}

zRPC_scheduler *zRPC_client_get_context(zRPC_client *client) {
    return client->context;
}
