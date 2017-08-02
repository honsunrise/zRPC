//
// Created by zhswo on 2016/11/22.
//
#include <netdb.h>
#include <malloc.h>
#include <listener.h>
#include <assert.h>
#include "server_engine.h"
#include "support/socket_utils.h"

void *initialize(zRPC_scheduler *scheduler);

void start(void *engine_context, zRPC_pipe *pipe, const zRPC_inetaddr *addr);

int32_t stop(void *engine_context);

void release(void *engine_context);

const zRPC_server_engine_vtable tcp_server_engine_vtable = {
    "tcp_server",
    initialize,
    start,
    stop,
    release
};

typedef struct zRPC_listener_node {
  zRPC_listener *listener;
  zRPC_list_head listener_node;
}zRPC_listener_node;

typedef struct zRPC_tcp_server {
  zRPC_scheduler *scheduler;
  zRPC_list_head listener_list;
} zRPC_tcp_server;

void *initialize(zRPC_scheduler *scheduler) {
  zRPC_tcp_server *tcp_server = (zRPC_tcp_server *) malloc(sizeof(zRPC_tcp_server));
  zRPC_list_init(&tcp_server->listener_list);
  tcp_server->scheduler = scheduler;
  return tcp_server;
}


struct child_thread_param {
  int new_fd;
  zRPC_pipe *pipe;
};

static int child_io_thread(void *arg) {
  struct child_thread_param *param = arg;
  int new_fd = param->new_fd;
  zRPC_pipe *pipe = param->pipe;
  free(param);

  zRPC_scheduler *scheduler = zRPC_scheduler_create();
  zRPC_channel *channel;
  zRPC_channel_create(&channel, pipe, new_fd, scheduler);
  zRPC_scheduler_run(scheduler);
  return 0;
}

static void _listener_callback(int new_fd, void *param) {
  struct child_thread_param *thread_param = malloc(sizeof(struct child_thread_param));
  thread_param->new_fd = new_fd;
  thread_param->pipe = param;
  zRPC_thread_id thread_id;
  zRPC_thread_create(&thread_id, child_io_thread, thread_param, ZRPC_THREAD_FLAG_JOINABLE);
}

void start(void *engine_context, zRPC_pipe *pipe, const zRPC_inetaddr *addr) {
  zRPC_tcp_server *server = engine_context;
  zRPC_socket_mode smode;
  assert(zRPC_inetaddr_get_port(addr) != 0);
  zRPC_listener *listener = NULL;
  zRPC_inetaddr wild_addr_v4, wild_addr_v6;
  if (zRPC_inetaddr_is_wildcard(addr)) {
    zRPC_inetaddr_make_wildcards(addr, &wild_addr_v4, &wild_addr_v6);
    addr = &wild_addr_v6;
    zRPC_listener_create(&listener, &smode, *addr, _listener_callback, pipe, server->scheduler);
    if (listener != NULL) {
      if (smode == ZRPC_SOCKET_IPV4_V6) {
        zRPC_listener_node *node = malloc(sizeof(zRPC_listener_node));
        node->listener = listener;
        zRPC_list_add_tail(&node->listener_node, &server->listener_list);
        return;
      }
    }
    addr = &wild_addr_v4;
  }

  zRPC_listener_create(&listener, &smode, *addr, _listener_callback, pipe, server->scheduler);
  if (listener != NULL) {
    zRPC_listener_node *node = malloc(sizeof(zRPC_listener_node));
    node->listener = listener;
    zRPC_list_add_tail(&node->listener_node, &server->listener_list);
  }
}

int32_t stop(void *engine_context) {
  zRPC_tcp_server *server = engine_context;
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &server->listener_list) {
    zRPC_listener_node *listener_node = zRPC_list_entry(pos, zRPC_listener_node, listener_node);
    zRPC_scheduler_unregister_source(server->scheduler, &listener_node->listener->source);
  }
  return 0;
}

void release(void *engine_context) {
  free(engine_context);
}