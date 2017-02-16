//
// Created by zhswo on 2016/11/23.
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include <zRPC/ds/hashmap.h>
#include "zRPC/context.h"

static void *initialize(zRPC_context *);

static int add(zRPC_context *, zRPC_event *);

static int del(zRPC_context *, zRPC_event *);

static int dispatch(zRPC_context *, zRPC_timespec *ts);

static void uninitialize(zRPC_context *);

static int int_hash_function(void *key) {
    return (int)(key);
}

static int equals(void* keyA, void* keyB) {
    return keyA == keyB;
}

typedef struct zRPC_epoll_context {
    zRPC_context *context;
    int ep_fd;
    zRPC_hashmap *ep_evs;
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
    epoll_context->ep_evs = hashmapCreate(1024, int_hash_function, equals);
    return epoll_context;
}

static void uninitialize(zRPC_context *context) {
    if (context->event_engine_context) {
        hashmapFree(((zRPC_epoll_context*)context->event_engine_context)->ep_evs);
        close(((zRPC_epoll_context*)context->event_engine_context)->ep_fd);
        free(context->event_engine_context);
    }
}

static int add(zRPC_context *context, zRPC_event *event) {
    zRPC_epoll_context *epoll_context = context->event_engine_context;
    if (!(event->event_status & EVS_INIT)) {
        return -1;
    }
    struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd));
    if (evs == NULL) {
        evs = malloc(sizeof(struct epoll_event));
        evs->data.ptr = event->fd;
        evs->events = EPOLLET;
        if (event->event_type & EV_WRITE) {
            evs->events |= EPOLLOUT;
        }
        if (event->event_type & EV_READ) {
            evs->events |= EPOLLIN;
        }
        hashmapPut(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd), evs);
        epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_ADD, zRPC_fd_origin(event->fd), evs);
    } else {
        if (event->event_type & EV_WRITE) {
            evs->events |= EPOLLOUT;
        }
        if (event->event_type & EV_READ) {
            evs->events |= EPOLLIN;
        }
        epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, zRPC_fd_origin(event->fd), evs);
    }
    event->event_status = EVS_REGISTER;
    return 0;
}

static int del(zRPC_context *context, zRPC_event *event) {
    zRPC_epoll_context *epoll_context = context->event_engine_context;
    if (!(event->event_status & EVS_REGISTER)) {
        return -1;
    }
    struct epoll_event *evs = hashmapGet(epoll_context->ep_evs, (void *) zRPC_fd_origin(event->fd));
    if (event->event_type & EV_READ)
        evs->events &= ~EPOLLIN;
    if (event->event_type & EV_WRITE)
        evs->events &= ~EPOLLOUT;
    if (evs->events & (EPOLLIN | EPOLLOUT)) {
        epoll_ctl(epoll_context->ep_fd, EPOLL_CTL_MOD, zRPC_fd_origin(event->fd), evs);
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
        zRPC_fd *fd = events[i].data.ptr;
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
        zRPC_context_fd_event_happen(context, fd, res);
    }
    return (0);
}