//
// Created by zhsyourai on 11/28/16.
//

#include "client_engine.h"
#include "zRPC/client.h"

extern const zRPC_client_engine_vtable tcp_client_engine_vtable;

static const zRPC_client_engine_vtable *g_client_engines[] = {
    &tcp_client_engine_vtable,
};

struct zRPC_client {
  zRPC_scheduler *scheduler;
  const zRPC_client_engine_vtable *client_engine;
  void *client_engine_context;
  zRPC_pipe *pipe;
  zRPC_channel *channel;
  char *hostname;
};

zRPC_client *zRPC_client_create(zRPC_scheduler *scheduler, zRPC_pipe *pipe) {
  zRPC_client *client = (zRPC_client *) malloc(sizeof(zRPC_client));
  client->scheduler = scheduler;
  client->pipe = pipe;
  client->hostname = NULL;
  client->client_engine = g_client_engines[0];
  return client;
}

static void resolver_complete_callback(void *custom_arg, zRPC_resolved *resolved) {
  zRPC_client *client = custom_arg;
  for (int i = 0; i < resolved->inetaddres.naddrs;) {
    client->client_engine_context = client->client_engine->initialize(client->scheduler, client->pipe, &resolved->inetaddres.addrs[i]);
    client->client_engine->start(client->client_engine_context);
    return;
  }
}

void zRPC_client_connect(zRPC_client *client, const char *hostname) {
  zRPC_resolver_address(client->scheduler, client->hostname, resolver_complete_callback, client);
}

zRPC_channel *zRPC_client_get_channel(zRPC_client *client) {
  return client->channel;
}
