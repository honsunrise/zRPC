//
// Created by zhsyourai on 12/6/16.
//

#include "event.h"

zRPC_event *zRPC_event_create(void *event_info, zRPC_EVENT_TYPE event_type) {
  zRPC_event *event = malloc(sizeof(zRPC_event));
  event->event_type = event_type;
  event->source = event_info;
  return event;
}

void zRPC_event_destroy(zRPC_event *event) {
  if (event)
    free(event);
}