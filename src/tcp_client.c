//
// Created by zhsyourai on 11/28/16.
//

#include <zRPC/context.h>
#include "zRPC/tcp_client.h"
#include "zRPC/support/socket_utils.h"

struct zRPC_tcp_client {
    zRPC_client *client;
    zRPC_fd *fd;
    zRPC_inetaddr addr;
};

zRPC_tcp_client *zRPC_tcp_client_create(zRPC_client *client) {
    zRPC_tcp_client *tcp_client = (zRPC_tcp_client *) malloc(sizeof(zRPC_tcp_client));
    tcp_client->client = client;
    return tcp_client;
}

void zRPC_tpc_client_set_addr(zRPC_tcp_client *client, zRPC_inetaddr *addr) {
    client->addr = *addr;
}

typedef struct zRPC_warp_fd_callback_arg {
    void *arg1;

    void *(*callback)(void *);
} zRPC_warp_fd_callback_arg;

static void *warp_fd_callback(void *origin_arg) {
    zRPC_warp_fd_callback_arg *arg = origin_arg;
    return (*arg->callback)(arg->arg1);
};

static int prepare_socket(const zRPC_inetaddr *addr, int fd) {
    int err = 0;

    err = zRPC_set_socket_nonblocking(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_cloexec(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_low_latency(fd, 1);
    if (err != 0)
        goto error;
    err = zRPC_set_socket_no_sigpipe_if(fd);
    if (err != 0)
        goto error;
    goto done;

    error:
    if (fd >= 0) {
        close(fd);
    }
    done:
    return err;
}

zRPC_channel *zRPC_tcp_client_start(zRPC_tcp_client *tcp_client) {
    int origin_fd;
    zRPC_socket_mode s_mode;
    int err;
    zRPC_inetaddr addr6_v4mapped;
    zRPC_inetaddr addr4_copy;
    zRPC_fd *fd;
    char *addr_str;
    int error;
    zRPC_client *client = tcp_client->client;

    if (zRPC_inetaddr_is_v4_mapped(&tcp_client->addr)) {
        zRPC_inetaddr_to_v4_mapped(&tcp_client->addr, &addr6_v4mapped);
        tcp_client->addr = addr6_v4mapped;
    }

    error = zRPC_create_socket(&tcp_client->addr, SOCK_STREAM, 0, &s_mode, &origin_fd);
    if (error != 0) {
        return NULL;
    }
    if (s_mode == ZRPC_SOCKET_IPV4) {
        if (zRPC_inetaddr_is_v4_mapped(&tcp_client->addr)) {
            zRPC_inetaddr_to_v4_mapped(&tcp_client->addr, &addr4_copy);
            tcp_client->addr = addr4_copy;
        }
    }
    if ((error = prepare_socket(&tcp_client->addr, origin_fd)) != 0) {
        return NULL;
    }

    do {
        err = connect(origin_fd, (const struct sockaddr *) tcp_client->addr.addr, (socklen_t) tcp_client->addr.len);
    } while (err < 0 && errno == EINTR);

    addr_str = zRPC_inetaddr_to_uri(&tcp_client->addr);

    fd = zRPC_fd_create(origin_fd);

    zRPC_warp_fd_callback_arg *arg_write = malloc(sizeof(zRPC_warp_fd_callback_arg));
    zRPC_warp_fd_callback_arg *arg_read = malloc(sizeof(zRPC_warp_fd_callback_arg));
    zRPC_runnable *on_write =
            zRPC_runnable_create(warp_fd_callback, arg_write, zRPC_runnable_noting_callback);
    zRPC_runnable *on_read =
            zRPC_runnable_create(warp_fd_callback, arg_read, zRPC_runnable_noting_callback);

    tcp_client->fd = fd;

    zRPC_channel *channel;
    zRPC_channel_create(&channel, zRPC_client_get_pipe(client), fd, zRPC_client_get_context(client));

    arg_read->arg1 = channel;
    arg_read->callback = (void *(*)(void *)) zRPC_channel_event_on_read;

    arg_write->arg1 = channel;
    arg_write->callback = (void *(*)(void *)) zRPC_channel_event_on_write;

    zRPC_event *event = zRPC_event_fd_create(fd, EV_READ | EV_PERSIST, on_read, NULL);
    zRPC_context_register_event(zRPC_client_get_context(client), event);
    event = zRPC_event_fd_create(fd, EV_WRITE, NULL, on_write);
    zRPC_context_register_event(zRPC_client_get_context(client), event);

    done:
    free(addr_str);
    return channel;
}
