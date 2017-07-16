//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_EVENT_H
#define ZRPC_EVENT_H

#include <malloc.h>
#include <zRPC/support/timer.h>
#include "zRPC/ds/queue.h"
#include "zRPC/ds/list.h"
#include "fd.h"

typedef enum zRPC_EVENT_TYPE {
  EV_OPEN = 0x01,
  EV_READ = 0x02,
  EV_WRITE = 0x04,
  EV_CLOSE = 0x08,
  EV_TIMER = 0x10,
  EV_ERROR = 0x20,
  EV_PERSIST = 0x40,
} zRPC_EVENT_TYPE;

#define EVENT_TYPE_FD_MASK (EV_OPEN | EV_READ | EV_WRITE | EV_CLOSE)
#define EVENT_TYPE_TIMER_MASK (EV_TIMER)

typedef enum zRPC_EVENT_STATUS {
  EVS_INIT = 0x01,
  EVS_REGISTER = 0x02,
  EVS_ACTIVE = 0x04
} zRPC_EVENT_STATUS;

typedef struct zRPC_event {
  union {
    zRPC_sample_fd *fd;
    zRPC_timer *timer;
  };

  void *event_info;
  int event_happen;
  zRPC_EVENT_TYPE event_type;
  zRPC_EVENT_STATUS event_status;
  zRPC_runnable *callback;
  zRPC_list_head list_node_register;
  zRPC_list_head list_node_active;
  zRPC_list_head list_node_remove;
} zRPC_event;

typedef struct zRPC_pending_event {
  zRPC_event *event;
  int event_happen;
  zRPC_list_head list_node;
} zRPC_pending_event;

/* THIS function work for zRPC_event */

zRPC_event *zRPC_event_create(void *target,
                              zRPC_EVENT_TYPE event_type,
                              zRPC_runnable *callback);

void zRPC_event_destroy(zRPC_event *event);

#endif //ZRPC_EVENT_H
