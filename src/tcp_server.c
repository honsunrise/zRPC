//
// Created by zhswo on 2016/11/22.
//
#include <netdb.h>
#include <malloc.h>
#include "zRPC/scheduling.h"
#include "zRPC/server.h"
#include "zRPC/tcp_server.h"

typedef struct zRPC_listener {
    struct zRPC_tcp_server *server;
    struct zRPC_listener *next;
    struct zRPC_listener *sibling;
    zRPC_fd *fd;
    int is_sibling;
    int is_reuse_port;
    zRPC_inetaddr address;

    /* Callback for accept*/
    void *accept_cb_arg;

    void *(*accept_cb)(void *);
} zRPC_listener;

struct zRPC_tcp_server {
    zRPC_server *server;
    int has_so_reuse_port;
    zRPC_listener *listener_head;
    zRPC_listener *listener_tail;
    unsigned int listeners;
};

zRPC_tcp_server *zRPC_tcp_server_create(zRPC_server *server) {
    check_socket();
    zRPC_tcp_server *tcp_server = (zRPC_tcp_server *) malloc(sizeof(zRPC_tcp_server));
    tcp_server->has_so_reuse_port = has_so_reuse_port;
    tcp_server->listener_head = NULL;
    tcp_server->listener_tail = NULL;
    tcp_server->server = server;
    return tcp_server;
}

static int prepare_listen_socket(int fd, const zRPC_inetaddr *address, int so_reuse_port) {
    zRPC_inetaddr inetaddr_temp;
    int err = 0;

    if (so_reuse_port) {
        err = zRPC_set_socket_reuse_port(fd, 1);
        if (err != 0)
            goto error;
    }

    err = zRPC_set_socket_nonblocking(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_cloexec(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_low_latency(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_reuse_address(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_no_sigpipe_if(fd);
    if (err != 0)
        goto error;

    if (bind(fd, (struct sockaddr *) address->addr, (socklen_t) address->len) < 0) {
        err = -1;
        goto error;
    }

    if (listen(fd, 100) < 0) {
        err = -1;
        goto error;
    }

    inetaddr_temp.len = sizeof(struct sockaddr_storage);

    if (getsockname(fd, (struct sockaddr *) inetaddr_temp.addr,
                    (socklen_t *) &inetaddr_temp.len) < 0) {
        err = -1;
        goto error;
    }

    return 0;

    error:
    if (fd >= 0) {
        close(fd);
    }
    return err;
}

typedef struct zRPC_warp_fd_callback_arg {
    void *arg1;

    void *(*callback)(void *);
} zRPC_warp_fd_callback_arg;

static void *warp_fd_callback(void *origin_arg) {
    zRPC_warp_fd_callback_arg *arg = origin_arg;
    return (*arg->callback)(arg->arg1);
};

struct child_thread_param {
    zRPC_server *server;
    zRPC_fd *new_fd;
};

static int child_io_thread(void *arg) {
    struct child_thread_param *param = arg;
    zRPC_server *server = param->server;
    zRPC_fd *new_fd = param->new_fd;
    free(param);

    zRPC_scheduler *context = zRPC_context_create();
    zRPC_channel *channel;
    zRPC_channel_create(&channel, zRPC_server_get_pipe(server), new_fd, context);

    zRPC_warp_fd_callback_arg *arg_write = malloc(sizeof(zRPC_warp_fd_callback_arg));
    zRPC_warp_fd_callback_arg *arg_read = malloc(sizeof(zRPC_warp_fd_callback_arg));
    zRPC_runnable *on_write = zRPC_runnable_create(warp_fd_callback, arg_write, zRPC_runnable_noting_callback);
    zRPC_runnable *on_read = zRPC_runnable_create(warp_fd_callback, arg_read, zRPC_runnable_noting_callback);

    arg_read->arg1 = channel;
    arg_read->callback = (void *(*)(void *)) zRPC_channel_event_on_read;

    arg_write->arg1 = channel;
    arg_write->callback = (void *(*)(void *)) zRPC_channel_event_on_write;

    zRPC_event *event = zRPC_event_create(new_fd, EV_READ | EV_PERSIST, on_read);
    zRPC_context_register_event(context, event);
    event = zRPC_event_create(new_fd, EV_WRITE, on_write);
    zRPC_context_register_event(context, event);
    zRPC_server_add_channel(server, channel);

    zRPC_context_dispatch(context);
    return 0;
}

static void *on_read(void *arg) {
    int err;
    zRPC_listener *listener = arg;
    zRPC_tcp_server *tcp_server = listener->server;
    zRPC_server *server = tcp_server->server;
    zRPC_fd *fd;

    /* loop until accept4 returns EAGAIN, and then re-arm notification */
    for (;;) {
        zRPC_inetaddr address;
        char *addr_str;
        address.len = sizeof(struct sockaddr_storage);
        int new_fd = zRPC_accept4(zRPC_fd_origin(listener->fd), &address, 1, 1);
        if (new_fd < 0) {
            switch (errno) {
                case EINTR:
                    continue;
                case EAGAIN:
                    return NULL;
                default:
                    goto error;
            }
        }

        zRPC_set_socket_no_sigpipe_if(new_fd);
        addr_str = zRPC_inetaddr_to_uri(&address);
        free(addr_str);

        fd = zRPC_fd_create(new_fd);
        struct child_thread_param *param = malloc(sizeof(struct child_thread_param ));
        param->new_fd = fd;
        param->server = server;
        zRPC_thread_id thread_id;
        zRPC_thread_create(&thread_id, child_io_thread, param, ZRPC_THREAD_FLAG_JOINABLE);
    }
    error:
    return NULL;
}

static int add_listener_to_server(zRPC_tcp_server *server, int fd, zRPC_inetaddr *address, zRPC_listener **out) {
    zRPC_listener *listener = (zRPC_listener *) malloc(sizeof(zRPC_listener));
    listener->server = server;
    listener->address = *address;
    listener->is_reuse_port = 0;
    listener->next = NULL;
    prepare_listen_socket(fd, &listener->address, listener->is_reuse_port);
    zRPC_runnable *read = zRPC_runnable_create(on_read, listener, zRPC_runnable_noting_callback);
    listener->fd = zRPC_fd_create(fd);
    zRPC_channel *channel;
    zRPC_channel_create(&channel, zRPC_server_get_pipe(server->server), listener->fd,
                        zRPC_server_get_context(server->server));
    zRPC_event *event = zRPC_event_create(listener->fd, EV_READ | EV_PERSIST, read);
    zRPC_context_register_event(zRPC_server_get_context(server->server), event);
    if (server->listener_head == NULL) {
        server->listener_head = listener;
    } else {
        server->listener_tail->next = listener;
    }
    server->listener_tail = listener;
    *out = listener;
    return 0;
}


void zRPC_tcp_server_add_listener(zRPC_tcp_server *server, zRPC_inetaddr *address) {
    zRPC_socket_mode smode;
    int fd;
    int errs[2];

    if (zRPC_inetaddr_get_port(address) == 0) {
        //TODO: report error
    }

    zRPC_listener *listener = NULL, *listener2 = NULL;
    zRPC_inetaddr wild_addr_v4, wild_addr_v6;
    if (zRPC_inetaddr_is_wildcard(address)) {
        zRPC_inetaddr_make_wildcards(address, &wild_addr_v4, &wild_addr_v6);

        /* Try listening on IPv6 first. */
        address = &wild_addr_v6;
        errs[0] = zRPC_create_socket(address, SOCK_STREAM, 0, &smode, &fd);
        if (errs[0] == 0) {
            errs[0] = add_listener_to_server(server, fd, address, &listener);
            if (fd >= 0 && smode == ZRPC_SOCKET_IPV4_V6) {
                return;
            }
        }
        address = &wild_addr_v4;
    }

    zRPC_inetaddr address_v4;
    errs[1] = zRPC_create_socket(address, SOCK_STREAM, 0, &smode, &fd);
    if (errs[1] == 0) {
        if (smode == ZRPC_SOCKET_IPV4 && zRPC_inetaddr_is_v4_mapped(address)) {
            zRPC_inetaddr_to_v4_mapped(address, &address_v4);
            address = &address_v4;
        }
        listener2 = listener;
        errs[1] = add_listener_to_server(server, fd, address, &listener);
        if (listener2 != NULL && listener != NULL) {
            listener2->sibling = listener;
            listener->is_sibling = 1;
        }
    }
}

void zRPC_tcp_server_start(zRPC_tcp_server *server) {
    zRPC_listener *listener;
    listener = server->listener_head;
    while (listener != NULL) {
        listener = listener->next;
    }
}