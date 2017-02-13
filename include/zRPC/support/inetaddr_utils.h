//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_INETADDR_UTILS_H
#define ZRPC_INETADDR_UTILS_H

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

#define ZRPC_MAX_SOCKADDR_SIZE 128
typedef struct zRPC_inetaddr {
    char addr[ZRPC_MAX_SOCKADDR_SIZE];
    size_t len;
} zRPC_inetaddr;

typedef struct zRPC_inetaddres {
    size_t naddrs;
    zRPC_inetaddr *addrs;
} zRPC_inetaddres;

int zRPC_inetaddr_is_v4_mapped(const zRPC_inetaddr *addr);

int zRPC_inetaddr_to_v4_mapped(const zRPC_inetaddr *addr, zRPC_inetaddr *addr4_out);

int zRPC_inetaddr_to_v6_mapped(const zRPC_inetaddr *addr, zRPC_inetaddr *addr6_out);

int zRPC_inetaddr_is_wildcard(const zRPC_inetaddr *addr);

void zRPC_inetaddr_make_wildcards(const zRPC_inetaddr *addr, zRPC_inetaddr *wild4_out, zRPC_inetaddr *wild6_out);

void zRPC_inetaddr_make_wildcard_v4(const zRPC_inetaddr *addr, zRPC_inetaddr *wild_out);

void zRPC_inetaddr_make_wildcard_v6(const zRPC_inetaddr *addr, zRPC_inetaddr *wild_out);

int zRPC_inetaddr_get_port(const zRPC_inetaddr *addr);

int zRPC_inetaddr_set_port(const zRPC_inetaddr *addr, int port);

int zRPC_inetaddr_to_string(char **out, const zRPC_inetaddr *addr, int normalize);

char *zRPC_inetaddr_to_uri(const zRPC_inetaddr *addr);

int parse_ipv4(const char *addr, zRPC_inetaddr *out);

#endif //ZRPC_INETADDR_UTILS_H
