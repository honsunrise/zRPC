//
// Created by zhsyourai on 12/6/16.
//

#include "zRPC/event.h"

zRPC_event *zRPC_event_create(void *target,
                              zRPC_EVENT_TYPE event_type,
                              zRPC_runnable *callback) {
  zRPC_event *event = malloc(sizeof(zRPC_event));
  if (event_type & EVENT_TYPE_FD_MASK)
    event->fd = target;
  else
    event->timer = target;
  event->event_happen = 0;
  event->event_status = EVS_INIT;
  event->event_type = event_type;
  event->callback = callback;
  event->event_info = NULL;
  return event;
}

void zRPC_event_destroy(zRPC_event *event) {
  if (event) free(event);
}