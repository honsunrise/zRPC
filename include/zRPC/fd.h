//
// Created by zhsyourai on 12/6/16.
//

#ifndef ZRPC_FD_H
#define ZRPC_FD_H

#include <sys/types.h>
#include "zRPC/support/runnable.h"
#include "event.h"

typedef struct zRPC_fd {
  zRPC_event_source source;
  int origin_fd;
} zRPC_fd;

/* THIS function work for zRPC_fd */

zRPC_fd *zRPC_fd_create(int fd);

void zRPC_fd_destroy(zRPC_fd *fd);

int zRPC_fd_origin(zRPC_fd *fd);

void zRPC_fd_close(zRPC_fd *fd);

ssize_t zRPC_fd_read(zRPC_fd *fd, void *buf, size_t len);

ssize_t zRPC_fd_write(zRPC_fd *fd, void *buf, size_t len);

void zRPC_fd_register_listener(zRPC_fd *fd, zRPC_EVENT_TYPE event_type, zRPC_event_listener_callback callback);

void zRPC_fd_unregister_listener(zRPC_fd *fd, zRPC_EVENT_TYPE event_type, zRPC_event_listener_callback callback);

#endif //ZRPC_FD_H
