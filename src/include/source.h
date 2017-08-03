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

typedef void (*zRPC_notify_listener_change)(void *notify_param, struct zRPC_event_source *source, zRPC_list_head *event_listener_list);

typedef void (*zRPC_destroy_callback)(struct zRPC_event_source *source);

typedef struct zRPC_event_source {
  DECLARE_RTTI;
  zRPC_list_head node;
  zRPC_list_head event_listener_list;
  zRPC_list_head event_listener_remove_list;
  zRPC_notify_listener_change notify;
  void *notify_param;
  int attention_event;
  zRPC_destroy_callback destroy_callback;
  // DO NOT MOVE THIS FIELD
  intptr_t key;
} zRPC_event_source;

void zRPC_source_init(zRPC_event_source *source);

void zRPC_source_register_listener(zRPC_event_source *source,
                                   int event_type,
                                   int onece,
                                   zRPC_event_listener_callback callback,
                                   void *param);

void zRPC_source_unregister_listener(zRPC_event_source *source,
                                     int event_type,
                                     zRPC_event_listener_callback callback);

void zRPC_source__emit_event(zRPC_event_source *source, zRPC_event event);

void zRPC_source_destroy(zRPC_event_source *source);
#endif //ZRPC_SOURCE_H
