//
// Created by zhsyourai on 7/20/17.
//

#ifndef ZRPC_SOURCE_H
#define ZRPC_SOURCE_H

#include "ds/list.h"
#include "event.h"
#include "rtti.h"
struct zRPC_event_source;

typedef void (*zRPC_notify_listener_change)(void *notify_param, struct zRPC_event_source *source, zRPC_list_head *event_listener_list);

typedef void (*zRPC_emit_event)(struct zRPC_event_source *source, zRPC_event event);

typedef struct zRPC_event_source {
  DECLARE_RTTI;
  zRPC_list_head event_listener_list;
  zRPC_list_head event_listener_remove_list;
  zRPC_notify_listener_change notify;
  void *notify_param;
  zRPC_emit_event emit;
  int attention_event;
  // DO NOT MOVE THIS FIELD
  int key;
} zRPC_event_source;

void zRPC_source_init(zRPC_event_source *source);

void zRPC_source_register_listener(zRPC_event_source *source,
                                   zRPC_EVENT_TYPE event_type,
                                   int onece,
                                   zRPC_event_listener_callback callback);

void zRPC_source_unregister_listener(zRPC_event_source *source,
                                     zRPC_EVENT_TYPE event_type,
                                     zRPC_event_listener_callback callback);
#endif //ZRPC_SOURCE_H
