//
// Created by zhswo on 2016/11/24.
//

#include <stdlib.h>
#include "support/socket_utils.h"
#include "server_engine.h"
#include "zRPC/server.h"

extern const zRPC_server_engine_vtable tcp_server_engine_vtable;

static const zRPC_server_engine_vtable *g_server_engines[] = {
    &tcp_server_engine_vtable,
};

struct zRPC_server {
  struct zRPC_scheduler *context;
  zRPC_pipe *pipe;
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


zRPC_pipe *zRPC_server_get_pipe(zRPC_server *server) {
  return server->pipe;
}
