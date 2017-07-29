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
  struct zRPC_scheduler *scheduler;
  zRPC_pipe *pipe;
  const zRPC_server_engine_vtable *server_engine;
  void *server_engine_context;
};

zRPC_server *zRPC_server_create(struct zRPC_scheduler *scheduler, const char *addr, zRPC_pipe *pipe) {
  zRPC_server *server = (zRPC_server *) malloc(sizeof(zRPC_server));
  server->scheduler = scheduler;
  server->pipe = pipe;
  server->server_engine = g_server_engines[0];
  server->server_engine_context = server->server_engine->initialize(scheduler);
  char *host, *port;
  zRPC_split_host_port(addr, &host, &port);
  zRPC_inetaddr r_addr;
  parse_ipv4(addr, &r_addr);
  server->server_engine->setup(server->server_engine_context, pipe, &r_addr);
  return server;
}

void zRPC_server_start(zRPC_server *server) {
  server->server_engine->start(server->server_engine_context);
}

void zRPC_server_stop(zRPC_server *server) {
  server->server_engine->stop(server->server_engine_context);
}