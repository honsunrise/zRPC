//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_SOCKET_UTILS_H
#define ZRPC_SOCKET_UTILS_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "inetaddr_utils.h"

const char *zRPC_inet_ntop(int af, const void *src, char *dst, size_t size);

int zRPC_accept4(int sockfd, zRPC_inetaddr *inetaddr, int nonblocking, int cloexec);

int zRPC_set_socket_nonblocking(int sockfd, int non_blocking);

int zRPC_set_socket_cloexec(int sockfd, int close_on_exec);

int zRPC_set_socket_reuse_address(int sockfd, int reuse);

int zRPC_set_socket_reuse_port(int sockfd, int reuse);

int zRPC_set_socket_low_latency(int sockfd, int low_latency);

int zRPC_ipv6_loopback_available(void);

int zRPC_set_socket_no_sigpipe_if(int sockfd);

int zRPC_set_socket_ip_pktinfo_if(int sockfd);

int zRPC_set_socket_ipv6_recvpktinfo_if(int sockfd);

int zRPC_set_socket_sendbuf(int sockfd, int size);

int zRPC_set_socket_recvbuf(int sockfd, int size);

int zRPC_create_socket_pair(int domain, int type, int protocol, int fds[2]);

ssize_t zRPC_socket_read(int sockfd, void *buf, size_t len);

ssize_t zRPC_socket_write(int sockfd, void *buf, size_t len);

typedef enum zRPC_socket_mode {
    ZRPC_SOCKET_NONE,
    ZRPC_SOCKET_IPV4,
    ZRPC_SOCKET_IPV6,
    ZRPC_SOCKET_IPV4_V6
} zRPC_socket_mode;

int zRPC_create_socket(const zRPC_inetaddr *addr,
                       int type, int protocol,
                       zRPC_socket_mode *smode, int *sockfd);

int zRPC_join_host_port(char **out, const char *host, int port);

int zRPC_split_host_port(const char *name, char **host, char **port);

#endif //ZRPC_SOCKET_UTILS_H
