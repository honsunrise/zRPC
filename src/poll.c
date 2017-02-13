//
// Created by zhswo on 2016/11/23.
//

#include <memory.h>
#include <sys/poll.h>
#include <zRPC/context.h>

typedef struct zRPC_poll_context {
    zRPC_context *context;
    struct pollfd *fds;
    unsigned int nfds;
    unsigned int fds_cap;
    zRPC_fd *fd_map[65535][3];
} zRPC_poll_context;

#define CHOICE_EVENT_IDX(ev) (ev & (EV_READ | EV_WRITE) ? ev & EV_READ ? 0 : 1 : 2)

static void *initialize(zRPC_context *);

static int add(zRPC_context *, zRPC_event *);

static int del(zRPC_context *, zRPC_event *);

static int dispatch(zRPC_context *, zRPC_timespec *ts);

static void uninitialize(zRPC_context *);

const zRPC_event_engine_vtable poll_event_engine_vtable = {
        "poll",
        initialize,
        add,
        del,
        dispatch,
        uninitialize,
        sizeof(zRPC_poll_context)
};

static void *initialize(zRPC_context *context) {
    zRPC_poll_context *poll_context = malloc(sizeof(zRPC_poll_context));
    poll_context->context = context;
    poll_context->nfds = 0;
    poll_context->fds_cap = 32;
    poll_context->fds = calloc(poll_context->fds_cap, sizeof(struct pollfd));
    return poll_context;
}

static void uninitialize(zRPC_context *context) {
    if (context->event_engine_context)
        free(((zRPC_poll_context *) context->event_engine_context)->fds);
    free(context->event_engine_context);
}

static int add(zRPC_context *context, zRPC_event *event) {
    zRPC_poll_context *poll_context = context->event_engine_context;
    if (!(event->event_status & EVS_INIT)) {
        return -1;
    }
    if (poll_context->nfds >= poll_context->fds_cap) {
        unsigned int new_size = poll_context->fds_cap * 2;
        struct pollfd *new_fds = realloc(poll_context->fds, new_size * sizeof(zRPC_poll_context));
        if (new_fds == NULL) {
            return -1;
        }
        poll_context->fds = new_fds;
        poll_context->fds_cap = new_size;
    }
    short care = 0;
    if (event->event_type & EV_WRITE) {
        care |= POLLOUT;
    }
    if (event->event_type & EV_READ) {
        care |= POLLIN;
    }
    event->event_status = EVS_REGISTER;
    event->event_info = malloc(sizeof(int));
    if (event->event_info == NULL) {
        return -1;
    }
    *(int *) event->event_info = poll_context->nfds;
    poll_context->fd_map[zRPC_fd_origin(event->fd)][CHOICE_EVENT_IDX(event->event_status)] = event->fd;
    poll_context->fds[poll_context->nfds].fd = zRPC_fd_origin(event->fd);
    poll_context->fds[poll_context->nfds++].events = care;
    return 0;
}

static int del(zRPC_context *context, zRPC_event *event) {
    zRPC_poll_context *poll_context = context->event_engine_context;
    if (!(event->event_status & EVS_REGISTER)) {
        return -1;
    }
    int pollfd_idx = *(int *) event->event_info;
    struct pollfd *pfd = &poll_context->fds[pollfd_idx];
    if (event->event_type & EV_READ)
        pfd->events &= ~POLLIN;
    if (event->event_type & EV_WRITE)
        pfd->events &= ~POLLOUT;
    if (pfd->events)
        return 0;
    free(event->event_info);
    return 0;
}

static int dispatch(zRPC_context *context, zRPC_timespec *ts) {
    zRPC_poll_context *poll_context = context->event_engine_context;

    struct pollfd *event_set;
    nfds_t nfds = poll_context->nfds;
    event_set = poll_context->fds;
    int res = poll(event_set, nfds, zRPC_time_to_millis(*ts));
    if (res == -1) {
        if (errno != EINTR)
            return -1;
        return 0;
    }

    if (res == 0 || nfds == 0)
        return (0);

    for (int j = 0; j < nfds; j++) {
        int happen;
        happen = event_set[j].revents;
        if (!happen)
            continue;

        res = 0;

        /* If the file gets closed notify */
        if (happen & (POLLHUP | POLLERR | POLLNVAL))
            happen |= POLLIN | POLLOUT;
        if (happen & POLLIN)
            res |= EV_READ;
        if (happen & POLLOUT)
            res |= EV_WRITE;
        if (res == 0)
            continue;

        zRPC_context_fd_event_happen(context, poll_context->fd_map[event_set[j].fd][CHOICE_EVENT_IDX(res)], res);
    }
    return (0);
}