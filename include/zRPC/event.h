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
    EV_ACTIVE = 0x01,
    EV_READ = 0x02,
    EV_WRITE = 0x04,
    EV_INACTIVE = 0x08,
    EV_TIMER = 0x10,
    EV_PERSIST = 0x20
} zRPC_EVENT_TYPE;

#define EVENT_TYPE_FD_MASK (EV_ACTIVE | EV_READ | EV_WRITE | EV_INACTIVE)
#define EVENT_TYPE_TIMER_MASK (EV_TIMER)

typedef enum zRPC_EVENT_STATUS {
    EVS_INIT = 0x01,
    EVS_REGISTER = 0x02,
    EVS_ACTIVE = 0x04
} zRPC_EVENT_STATUS;

typedef struct zRPC_event {
    union {
        zRPC_fd *fd;
        zRPC_timer *timer;
    };

    void *event_info;
    int event_happen;
    zRPC_EVENT_TYPE event_type;
    zRPC_EVENT_STATUS event_status;
    union {
        struct {
            zRPC_runnable *read_callback;
            zRPC_runnable *write_callback;
            zRPC_runnable *close_callback;
        };
        struct {
            zRPC_runnable *callback;
        };
    };
    zRPC_list_head list_node_register;
    zRPC_list_head list_node_active;
} zRPC_event;

typedef struct zRPC_pending_event {
    zRPC_event *event;
    int event_happen;
    zRPC_list_head list_node;
} zRPC_pending_event;

/* THIS function work for zRPC_event */

zRPC_event *zRPC_event_fd_create(zRPC_fd *fd,
                                 zRPC_EVENT_TYPE event_type,
                                 zRPC_runnable *read_callback,
                                 zRPC_runnable *write_callback);

zRPC_event *zRPC_event_timer_create(zRPC_timer *timer,
                                    zRPC_runnable *callback);

void zRPC_event_destroy(zRPC_event *event);

void zRPC_event_set_event_type(zRPC_event *event, zRPC_EVENT_TYPE event_type);

#endif //ZRPC_EVENT_H
