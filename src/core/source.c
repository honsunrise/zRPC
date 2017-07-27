//
// Created by zhsyourai on 7/21/17.
//

#include "source.h"

void zRPC_source_init(zRPC_event_source *source) {
  source->key = (int) source;
  zRPC_list_init(&source->event_listener_list);
  zRPC_list_init(&source->event_listener_remove_list);
}

void zRPC_source_register_listener(zRPC_event_source *source,
                                   zRPC_EVENT_TYPE event_type,
                                   int onece,
                                   zRPC_event_listener_callback callback,
                                   void *param) {
  zRPC_event_listener *listener = malloc(sizeof(listener));
  listener->callback = callback;
  listener->event_type = event_type;
  listener->param = param;
  listener->onece = onece;
  zRPC_list_add_tail(&listener->list_node, &source->event_listener_list);
  source->notify(source->notify_param, source, &source->event_listener_list);
}

void zRPC_source_unregister_listener(zRPC_event_source *source,
                                     zRPC_EVENT_TYPE event_type,
                                     zRPC_event_listener_callback callback) {
  zRPC_list_head *pos;
  zRPC_event_listener *listener = NULL;
  zRPC_list_for_each(pos, &source->event_listener_list) {
    listener = zRPC_list_entry(pos, zRPC_event_listener, list_node);
    if(listener->callback == callback && listener->event_type == event_type) {
      break;
    }
  }
  if(listener != NULL) {
    zRPC_list_del(&listener->list_node);
    free(listener);
  }
}

void zRPC_source__emit_event(zRPC_event_source *source, zRPC_event event) {
  zRPC_list_head *pos;
  zRPC_event_listener *listener = NULL;
  zRPC_list_for_each(pos, &source->event_listener_list) {
    listener = zRPC_list_entry(pos, zRPC_event_listener, list_node);
    if(listener->event_type & event.event_type) {
      listener->callback(source, event, listener->param);
      if(listener->onece)
        zRPC_list_add_tail(&listener->remove_list_node, &source->event_listener_remove_list);
    }
  }
  listener = NULL;
  zRPC_list_for_each(pos, &source->event_listener_remove_list) {
    if(listener) {
      zRPC_list_del(&listener->list_node);
    }
    listener = zRPC_list_entry(pos, zRPC_event_listener, list_node);
  }
  if(listener) {
    zRPC_list_del(&listener->list_node);
  }
  zRPC_list_init(&source->event_listener_remove_list);
}