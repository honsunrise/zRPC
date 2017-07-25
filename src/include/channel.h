//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_CHANNEL_H
#define ZRPC_CHANNEL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ds/ring_buf.h"
#include "event.h"
#include "zRPC/filter.h"
#include "zRPC/scheduling.h"

struct zRPC_filter;

struct zRPC_filter_factory;

typedef struct zRPC_filter_linked_node {
  struct zRPC_filter_linked_node *next;
  struct zRPC_filter_linked_node *prev;
  struct zRPC_filter *filter;
} zRPC_filter_linked_node;

typedef struct zRPC_filter_factory_linked_node {
  const char *name;
  struct zRPC_filter_factory_linked_node *next;
  struct zRPC_filter_factory_linked_node *prev;
  struct zRPC_filter_factory *filter_factory;
} zRPC_filter_factory_linked_node;

typedef struct zRPC_pipe {
  zRPC_filter_factory_linked_node *head;
  zRPC_filter_factory_linked_node *tail;
  int filters;
} zRPC_pipe;

typedef struct zRPC_channel {
  zRPC_event_source source;
  zRPC_scheduler *scheduler;
  zRPC_pipe *pipe;
  zRPC_filter_linked_node *head;
  zRPC_filter_linked_node *tail;
  int fd;
  zRPC_ring_buffer *buffer;
  int is_active;
  void *custom_data;
  int is_writing;
  zRPC_list_head pending_write;
  zRPC_list_head done_write;
  zRPC_mutex write_lock;
  zRPC_mutex read_lock;
} zRPC_channel;

void zRPC_pipe_create(zRPC_pipe **out);

void zRPC_pipe_destroy(zRPC_pipe *pipe);

void zRPC_pipe_add_filter_with_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory *filter_factory);

void zRPC_pipe_add_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory);

void zRPC_pipe_remove_filter_by_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory **filter_factory);

void zRPC_pipe_remove_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory);

void zRPC_channel_create(zRPC_channel **out, zRPC_pipe *pipe, int fd, zRPC_scheduler *scheduler);

void zRPC_channel_destroy(zRPC_channel *channel);

void zRPC_channel_set_custom_data(zRPC_channel *channel, void *custom_data);

void *zRPC_channel_get_custom_data(zRPC_channel *channel);

void zRPC_channel_write(zRPC_channel *channel, void *msg);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CHANNEL_H
