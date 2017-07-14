//
// Created by zhsyourai on 12/29/16.
//

#include <sys/socket.h>
#include "zRPC/support/socket_utils.h"
#include "fd_notifiable.h"

int zRPC_create_notifiable_fd(zRPC_sample_fd *fds[2]) {
    int fd[2];
    fds[0] = NULL;
    fds[1] = NULL;
    if (zRPC_create_socket_pair(AF_UNIX, SOCK_STREAM, 0, fd) == 0) {
        if (zRPC_set_socket_nonblocking(fd[0], 1) < 0 ||
            zRPC_set_socket_nonblocking(fd[1], 1) < 0 ||
            zRPC_set_socket_cloexec(fd[0], 1) < 0 ||
            zRPC_set_socket_cloexec(fd[0], 1) < 0) {
            close(fd[0]);
            close(fd[1]);
            fd[0] = fd[1] = -1;
            return -1;
        }
        fds[0] = zRPC_fd_create(fd[0]);
        fds[1] = zRPC_fd_create(fd[1]);
        return 0;
    }
    fd[0] = fd[1] = -1;
    return -1;
}
