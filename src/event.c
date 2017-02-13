//
// Created by zhsyourai on 12/6/16.
//

#include "zRPC/event.h"

zRPC_event *zRPC_event_fd_create(zRPC_fd *fd,
                                 zRPC_EVENT_TYPE event_type,
                                 zRPC_runnable *read_callback,
                                 zRPC_runnable *write_callback) {
    zRPC_event *event = malloc(sizeof(zRPC_event));
    event->fd = fd;
    event->event_happen = 0;
    event->event_status = EVS_INIT;
    event->event_type = event_type;
    event->read_callback = read_callback;
    event->write_callback = write_callback;
    event->event_info = NULL;
    return event;
}

zRPC_event *zRPC_event_timer_create(zRPC_timer *timer,
                                    zRPC_runnable *callback) {
    zRPC_event *event = malloc(sizeof(zRPC_event));
    event->timer = timer;
    event->event_happen = 0;
    event->event_status = EVS_INIT;
    event->event_type = EV_TIMER;
    event->callback = callback;
    event->event_info = NULL;
    return event;
}

void zRPC_event_destroy(zRPC_event *event) {
    if (event) free(event);
}

void zRPC_event_set_event_type(zRPC_event *event, zRPC_EVENT_TYPE event_type) {
    event->event_type = event_type;
}