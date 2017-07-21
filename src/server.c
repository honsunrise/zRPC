//
// Created by zhswo on 2016/11/24.
//

#include <stdlib.h>
#include "zRPC/server.h"
#include "zRPC/tcp_server.h"

struct zRPC_server {
    struct zRPC_scheduler *context;
    zRPC_pipe *pipe;
    zRPC_channel **channel;
    unsigned int channel_cap;
    unsigned int channel_count;
    zRPC_tcp_server *tcp_server;
};

#define INIT_CHANNEL_COUNT 50

zRPC_server *zRPC_server_create(struct zRPC_scheduler *context, const char *addr, zRPC_pipe *pipe) {
    zRPC_server *server = (zRPC_server *) malloc(sizeof(zRPC_server));
    server->context = context;
    server->pipe = pipe;
    char *host, *port;
    zRPC_split_host_port(addr, &host, &port);
    server->tcp_server = zRPC_tcp_server_create(server);
    server->channel_count = 0;
    server->channel_cap = INIT_CHANNEL_COUNT;
    server->channel = malloc(sizeof(*server->channel) * INIT_CHANNEL_COUNT);
    zRPC_inetaddr r_addr;
    parse_ipv4(addr, &r_addr);
    zRPC_tcp_server_add_listener(server->tcp_server, &r_addr);
    return server;
}

void zRPC_server_start(zRPC_server *server) {
    zRPC_tcp_server_start(server->tcp_server);
}

void zRPC_server_add_channel(zRPC_server *server, zRPC_channel *channel) {
    if (server->channel_count >= server->channel_cap) {
        server->channel_count += 5;
        server->channel = realloc(server->channel, sizeof(*server->channel) * server->channel_count);
    }
    server->channel[server->channel_count++] = channel;
}

void zRPC_server_get_channels(zRPC_server *server, zRPC_channel ***out, unsigned int *count) {
    *out = server->channel;
    *count = server->channel_count;
}

struct zRPC_scheduler *zRPC_server_get_context(zRPC_server *server) {
    return server->context;
}

zRPC_pipe *zRPC_server_get_pipe(zRPC_server *server) {
    return server->pipe;
}
