//
// Created by zhswo on 2016/11/23.
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include "zRPC/context.h"

static void *initialize(zRPC_context *);

static int add(zRPC_context *, zRPC_event *);

static int del(zRPC_context *, zRPC_event *);

static int dispatch(zRPC_context *, zRPC_timespec *ts);

static void uninitialize(zRPC_context *);

typedef struct zRPC_epoll_context {
    zRPC_context *context;
    int ep_fd;
    struct epoll_event *ep_evs;
    size_t ep_evs_cap;
    size_t ep_evs_num;
    int ep_events;
} zRPC_epoll_context;

const zRPC_event_engine_vtable epoll_event_engine_vtable = {
        "epoll",
        initialize,
        add,
        del,
        dispatch,
        uninitialize,
        sizeof(zRPC_epoll_context)
};

static void *initialize(zRPC_context *context) {
    zRPC_epoll_context *epoll_context = malloc(sizeof(zRPC_epoll_context));
    epoll_context->context = context;
    epoll_context->ep_fd = epoll_create1(EPOLL_CLOEXEC);
    epoll_context->ep_evs_cap = 32;
    epoll_context->ep_evs = malloc(sizeof(struct epoll_event) * epoll_context->ep_evs_cap);
    epoll_context->ep_evs_num = 0;
    return epoll_context;
}

static void uninitialize(zRPC_context *context) {
    if (context->event_engine_context) {
        free(((zRPC_epoll_context*)context->event_engine_context)->ep_evs);
        free(context->event_engine_context);
    }
}

static int add(zRPC_context *context, zRPC_event *event) {
    zRPC_epoll_context *epoll_context = context->event_engine_context;
    if (epoll_context->ep_evs_num >= epoll_context->ep_evs_cap) {
        size_t new_size = epoll_context->ep_evs_cap * 2;
        struct epoll_event *new_ep_evs = realloc(epoll_context->ep_evs, new_size * sizeof(struct epoll_event));
        if (new_ep_evs == NULL) {
            return -1;
        }
        epoll_context->ep_evs = new_ep_evs;
        epoll_context->ep_evs_cap = new_size;
    }
    if (!(event->event_status & EVS_INIT)) {
        return -1;
    }
    epoll_context->ep_evs[epoll_context->ep_evs_num].data.ptr = event;
    epoll_context->ep_evs[epoll_context->ep_evs_num].events = EPOLLET;
    if (event->event_type & EV_WRITE) {
        epoll_context->ep_evs[epoll_context->ep_evs_num].events |= EPOLLOUT;
    }
    if (event->event_type & EV_READ) {
        epoll_context->ep_evs[epoll_context->ep_evs_num].events |= EPOLLIN;
    }
    event->event_status = EVS_REGISTER;
    event->event_info = malloc(sizeof(int));
    if (event->event_info == NULL) {
        return -1;
    }
    *(int *) event->event_info = (int) epoll_context->ep_evs_num;
    epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_ADD, zRPC_fd_origin(event->fd),
              &epoll_context->ep_evs[epoll_context->ep_evs_num]);
    ++epoll_context->ep_evs_num;
    return 0;
}

static int del(zRPC_context *context, zRPC_event *event) {
    zRPC_epoll_context *epoll_context = context->event_engine_context;
    if (!(event->event_status & EVS_REGISTER)) {
        return -1;
    }
    int ep_evs_index = *(int *) event->event_info;
    struct epoll_event *e_v = &epoll_context->ep_evs[ep_evs_index];
    if (event->event_type & EV_READ)
        e_v->events &= ~EPOLLIN;
    if (event->event_type & EV_WRITE)
        e_v->events &= ~EPOLLOUT;
    if (e_v->events & (EPOLLIN | EPOLLOUT)) {
        epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, zRPC_fd_origin(event->fd), e_v);
        return 0;
    } else {
        epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_DEL, zRPC_fd_origin(event->fd), NULL);
        free(event->event_info);
        return 0;
    }
}

#define MAX_EVENTS 128

static int dispatch(zRPC_context *context, zRPC_timespec *ts) {
    zRPC_epoll_context *epoll_context = context->event_engine_context;
    struct epoll_event events[MAX_EVENTS];
    struct pollfd *event_set;
    int ep_rv = epoll_wait(epoll_context->ep_fd, events, MAX_EVENTS, zRPC_time_to_millis(*ts));
    for (int i = 0; i < ep_rv; ++i) {
        void *data_ptr = events[i].data.ptr;
        zRPC_event *event = data_ptr;
        int cancel = events[i].events & (EPOLLERR | EPOLLHUP);
        int read_ev = events[i].events & (EPOLLIN | EPOLLPRI);
        int write_ev = events[i].events & EPOLLOUT;
        int res = 0;
        if (read_ev || cancel) {
            res |= EV_READ;
        }
        if (write_ev || cancel) {
            res |= EV_WRITE;
        }
        if (res == 0) {
            continue;
        }
        zRPC_context_fd_event_happen(context, event->fd, res);
    }
    return (0);
}