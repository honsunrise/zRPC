//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_EVENT_H
#define ZRPC_EVENT_H

#include <malloc.h>
#include "ds/queue.h"
#include "ds/list.h"

typedef enum zRPC_EVENT_TYPE {
  EV_OPEN = 0x01,
  EV_READ = 0x02,
  EV_WRITE = 0x04,
  EV_CLOSE = 0x08,
  EV_TIMER = 0x10,
  EV_ERROR = 0x20
} zRPC_EVENT_TYPE;

#define EVENT_TYPE_FD_MASK (EV_OPEN | EV_READ | EV_WRITE | EV_CLOSE)
#define EVENT_TYPE_TIMER_MASK (EV_TIMER)

typedef enum zRPC_EVENT_STATUS {
  EVS_INIT = 0x01,
  EVS_REGISTER = 0x02,
  EVS_ACTIVE = 0x04
} zRPC_EVENT_STATUS;

typedef struct zRPC_event {
  void *event_info;
  zRPC_EVENT_TYPE event_type;
} zRPC_event;

typedef void (*zRPC_event_listener_callback)(void *source, zRPC_event event, void *param);

typedef struct zRPC_event_listener {
  int event_type;
  int onece;
  zRPC_list_head list_node;
  zRPC_list_head remove_list_node;
  zRPC_event_listener_callback callback;
  void *param;
} zRPC_event_listener;

/* THIS function work for zRPC_event */

zRPC_event *zRPC_event_create(void *event_info, zRPC_EVENT_TYPE event_type);

void zRPC_event_destroy(zRPC_event *event);

#endif //ZRPC_EVENT_H
