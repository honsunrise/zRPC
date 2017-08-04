//
// Created by zhsyourai on 7/20/17.
//

#ifndef ZRPC_SOURCE_H
#define ZRPC_SOURCE_H

#include <stdint.h>
#include "ds/list.h"
#include "event.h"
#include "rtti.h"
struct zRPC_event_source;
struct zRPC_scheduler;

typedef enum zRPC_notify_type {
  LISTENER_REGISTER,
  LISTENER_UNREGISTER,
}zRPC_notify_type;

typedef void
(*zRPC_notify_listener_change)(zRPC_notify_type type, struct zRPC_event_source *source, void *notify_param);

typedef void (*zRPC_destroy_callback)(struct zRPC_event_source *source);

typedef struct zRPC_event_source {
  DECLARE_RTTI;
  zRPC_list_head node;
  zRPC_list_head node_destroy;
  zRPC_list_head event_listener_list;
  zRPC_list_head event_listener_remove_list;
  zRPC_notify_listener_change notify;
  struct zRPC_scheduler *scheduler;
  int attention_event;
  zRPC_destroy_callback destroy_callback;
  // DO NOT MOVE THIS FIELD
  intptr_t key;
} zRPC_event_source;

void zRPC_source_init(zRPC_event_source *source);

zRPC_event_listener* zRPC_source_register_listener(zRPC_event_source *source,
                                   int event_type,
                                   int onece,
                                   zRPC_event_listener_callback callback,
                                   void *param);

void zRPC_source_unregister_listener(zRPC_event_source *source,
                                     zRPC_event_listener *listener);

void zRPC_source_emit_event(zRPC_event_source *source, zRPC_event event);

void zRPC_source_destroy(zRPC_event_source *source);
#endif //ZRPC_SOURCE_H
