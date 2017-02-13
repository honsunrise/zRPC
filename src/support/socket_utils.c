//
// Created by zhswo on 2016/11/23.
//
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include "zRPC/support/string_utils.h"
#include "zRPC/support/socket_utils.h"

const char *zRPC_inet_ntop(int af, const void *src, char *dst, size_t size) {
    return inet_ntop(af, src, dst, (socklen_t) size);
}

int zRPC_accept4(int sockfd, zRPC_inetaddr *inetaddr, int nonblocking, int cloexec) {
    int new_sock_fd, flags;
    new_sock_fd = accept(sockfd, (struct sockaddr *) inetaddr->addr, (socklen_t *) &inetaddr->len);
    if (new_sock_fd >= 0) {
        if (nonblocking) {
            flags = fcntl(new_sock_fd, F_GETFL, 0);
            if (flags < 0) goto close_and_error;
            if (fcntl(new_sock_fd, F_SETFL, flags | O_NONBLOCK) != 0) goto close_and_error;
        }
        if (cloexec) {
            flags = fcntl(new_sock_fd, F_GETFD, 0);
            if (flags < 0) goto close_and_error;
            if (fcntl(new_sock_fd, F_SETFD, flags | FD_CLOEXEC) != 0) goto close_and_error;
        }
    }
    return new_sock_fd;

    close_and_error:
    close(new_sock_fd);
    return -1;
}

int zRPC_set_socket_nonblocking(int sockfd, int non_blocking) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(sockfd, F_SETFL, flags) != 0) {
        return -1;
    }

    return 0;
}

int zRPC_set_socket_cloexec(int sockfd, int close_on_exec) {
    int flags = fcntl(sockfd, F_GETFD, 0);
    if (flags < 0) {
        return -1;
    }

    if (close_on_exec) {
        flags |= FD_CLOEXEC;
    } else {
        flags &= ~FD_CLOEXEC;
    }

    if (fcntl(sockfd, F_SETFD, flags) != 0) {
        return -1;
    }

    return 0;
}

int zRPC_set_socket_reuse_address(int sockfd, int reuse) {
    int v = (reuse != 0);
    int nv;
    socklen_t int_len = sizeof(nv);
    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v))) {
        return -1;
    }
    if (0 != getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &nv, &int_len)) {
        return -1;
    }
    if ((0 != nv) != v) {
        return -1;
    }

    return 0;
}

// #define SUPPORT_SO_REUSEPORT
int zRPC_set_socket_reuse_port(int sockfd, int reuse) {
#ifdef SUPPORT_SO_REUSEPORT
    int v = (reuse != 0);
    int nv;
    socklen_t int_len = sizeof(nv);
    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v))) {
        return -1;
    }
    if (0 != getsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &nv, &int_len)) {
        return -1;
    }
    if ((0 != nv) != v) {
        return -1;
    }
#endif
    return 0;
}

int zRPC_set_socket_low_latency(int sockfd, int low_latency) {
    int v = (low_latency != 0);
    int nv;
    socklen_t int_len = sizeof(nv);
    if (0 != setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v))) {
        return -1;
    }
    if (0 != getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nv, &int_len)) {
        return -1;
    }
    if ((0 != nv) != v) {
        return -1;
    }
    return 0;
}

static int g_is_probe_ipv6 = 0;
static int g_ipv6_loopback_available;

static void probe_ipv6(void) {
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    g_ipv6_loopback_available = 0;
    if (sockfd > 0) {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr.s6_addr[15] = 1;
        if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
            g_ipv6_loopback_available = 1;
        }
        close(sockfd);
    }
}

int zRPC_ipv6_loopback_available(void) {
    if (!g_is_probe_ipv6) {
        probe_ipv6();
        g_is_probe_ipv6 = 1;
    }
    return g_ipv6_loopback_available;
}

int zRPC_set_socket_no_sigpipe_if(int sockfd) {
    int v = 1;
    int nv;
    socklen_t int_len = sizeof(nv);
    return 0;
#ifdef __linux__
    if (0 != setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &v, sizeof(v))) {
        return -1;
    }
    if (0 != getsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &nv, &int_len)) {
        return -1;
    }
#else
    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_NOSIGNAL, &v, sizeof(v))) {
        return -1;
    }
    if (0 != getsockopt(sockfd, SOL_SOCKET, SO_NOSIGNAL, &nv, &int_len)) {
        return -1;
    }
#endif

    if ((nv != 0) != (v != 0)) {
        return -1;
    }
    return 0;
}

int zRPC_set_socket_ip_pktinfo_if(int sockfd) {
    int v = 1;
    if (0 != setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &v, sizeof(v))) {
        return -1;
    }
    return 0;
}

int zRPC_set_socket_ipv6_recvpktinfo_if(int sockfd) {
    int v = 1;
    if (0 != setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &v, sizeof(v))) {
        return -1;
    }
    return 0;
}

int zRPC_set_socket_sendbuf(int sockfd, int size) {
    return 0 == setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) ? 0 : -1;
}

int zRPC_set_socket_recvbuf(int sockfd, int size) {
    return 0 == setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) ? 0 : -1;
}

static int set_socket_both(int sockfd) {
    const int off = 0;
    return 0 == setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
}

int zRPC_create_socket(const zRPC_inetaddr *inetaddr,
                       int type, int protocol,
                       zRPC_socket_mode *smode, int *sockfd) {
    const struct sockaddr *addr = (const struct sockaddr *) inetaddr->addr;
    int family = addr->sa_family;
    if (family == AF_INET6) {
        if (zRPC_ipv6_loopback_available()) {
            *sockfd = socket(family, type, protocol);
        } else {
            *sockfd = -1;
            errno = EAFNOSUPPORT;
        }
        if (*sockfd >= 0 && set_socket_both(*sockfd)) {
            *smode = ZRPC_SOCKET_IPV4_V6;
            return 0;
        }
        /* If this isn't an IPv4 address, then return whatever we've got. */
        if (!zRPC_inetaddr_is_v4_mapped(inetaddr)) {
            *smode = ZRPC_SOCKET_IPV6;
            return *sockfd >= 0 ? 0 : errno;
        }

        if (*sockfd >= 0) {
            close(*sockfd);
        }
        family = AF_INET;
    }
    *smode = family == AF_INET ? ZRPC_SOCKET_IPV4 : ZRPC_SOCKET_NONE;
    *sockfd = socket(family, type, protocol);
    return *sockfd >= 0 ? 0 : errno;
}

int zRPC_create_socket_pair(int domain, int type, int protocol, int *fds) {
    return socketpair(domain, type, protocol, fds);
}


int zRPC_join_host_port(char **out, const char *host, int port) {
    if (host[0] != '[' && strchr(host, ':') != NULL) {
        /* IPv6 literals must be enclosed in brackets. */
        return zRPC_sprintf_out(out, "[%s]:%d", host, port);
    } else {
        /* Ordinary non-bracketed host:port. */
        return zRPC_sprintf_out(out, "%s:%d", host, port);
    }
}

int zRPC_split_host_port(const char *name, char **host, char **port) {
    const char *host_start;
    size_t host_len;
    const char *port_start;

    *host = NULL;
    *port = NULL;

    if (name[0] == '[') {
        /* Parse a bracketed host, typically an IPv6 literal. */
        const char *rbracket = strchr(name, ']');
        if (rbracket == NULL) {
            /* Unmatched [ */
            return 0;
        }
        if (rbracket[1] == '\0') {
            /* ]<end> */
            port_start = NULL;
        } else if (rbracket[1] == ':') {
            /* ]:<port?> */
            port_start = rbracket + 2;
        } else {
            /* ]<invalid> */
            return 0;
        }
        host_start = name + 1;
        host_len = (size_t) (rbracket - host_start);
        if (memchr(host_start, ':', host_len) == NULL) {
            /* Require all bracketed hosts to contain a colon, because a hostname or
               IPv4 address should never use brackets. */
            return 0;
        }
    } else {
        const char *colon = strchr(name, ':');
        if (colon != NULL && strchr(colon + 1, ':') == NULL) {
            /* Exactly 1 colon.  Split into host:port. */
            host_start = name;
            host_len = (size_t) (colon - name);
            port_start = colon + 1;
        } else {
            /* 0 or 2+ colons.  Bare hostname or IPv6 litearal. */
            host_start = name;
            host_len = strlen(name);
            port_start = NULL;
        }
    }

    /* Allocate return values. */
    *host = malloc(host_len + 1);
    memcpy(*host, host_start, host_len);
    (*host)[host_len] = '\0';

    if (port_start != NULL) {
        *port = zRPC_str_dup(port_start);
    }

    return 1;
}
