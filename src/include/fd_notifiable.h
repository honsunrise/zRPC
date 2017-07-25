//
// Created by zhsyourai on 12/29/16.
//

#ifndef ZRPC_FD_NOTIFIABLE_H
#define ZRPC_FD_NOTIFIABLE_H

int zRPC_create_notifiable_fd(int fds[2]);

int zRPC_destroy_notifiable_fd(int fds[2]);

#endif //ZRPC_FD_NOTIFIABLE_H
