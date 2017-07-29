//
// Created by zhswo on 2016/11/23.
//

#include <malloc.h>
#include "support/socket_utils.h"

static const uint8_t v6_map_v4_prefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

int zRPC_inetaddr_is_v4_mapped(const zRPC_inetaddr *address) {
    const struct sockaddr *addr = (const struct sockaddr *) address->addr;
    if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *) addr;
        if (memcmp(addr6->sin6_addr.s6_addr, v6_map_v4_prefix, sizeof(v6_map_v4_prefix)) == 0) {
            return 1;
        }
    }
    return 0;
}


//if (resolved_addr4_out != NULL) {
///* Normalize ::ffff:0.0.0.0/96 to IPv4. */
//memset(resolved_addr4_out, 0, sizeof(*resolved_addr4_out));
//addr4_out->sin_family = AF_INET;
///* s6_addr32 would be nice, but it's non-standard. */
//memcpy(&addr4_out->sin_addr, &addr6->sin6_addr.s6_addr[12], 4);
//addr4_out->sin_port = addr6->sin6_port;
//resolved_addr4_out->len = sizeof(struct sockaddr_in);
//}

int zRPC_inetaddr_to_v4_mapped(const zRPC_inetaddr *address, zRPC_inetaddr *resolved_addr6_out) {
    const struct sockaddr *addr = (const struct sockaddr *) address->addr;
    struct sockaddr_in6 *addr6_out = (struct sockaddr_in6 *) resolved_addr6_out->addr;
    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr4 = (const struct sockaddr_in *) addr;
        memset(resolved_addr6_out, 0, sizeof(*resolved_addr6_out));
        addr6_out->sin6_family = AF_INET6;
        memcpy(&addr6_out->sin6_addr.s6_addr[0], v6_map_v4_prefix, 12);
        memcpy(&addr6_out->sin6_addr.s6_addr[12], &addr4->sin_addr, 4);
        addr6_out->sin6_port = addr4->sin_port;
        resolved_addr6_out->len = sizeof(struct sockaddr_in6);
        return 1;
    }
    return 0;
}

int zRPC_inetaddr_is_wildcard(const zRPC_inetaddr *address) {
    const struct sockaddr *addr;
    zRPC_inetaddr addr4_normalized;
    if (zRPC_inetaddr_is_v4_mapped(address)) {
        zRPC_inetaddr_to_v4_mapped(address, &addr4_normalized);
        address = &addr4_normalized;
    }
    addr = (const struct sockaddr *) address->addr;
    if (addr->sa_family == AF_INET) {
        /* Check for 0.0.0.0 */
        const struct sockaddr_in *addr4 = (const struct sockaddr_in *) addr;
        if (addr4->sin_addr.s_addr != 0) {
            return 0;
        }
        return 1;
    } else if (addr->sa_family == AF_INET6) {
        /* Check for :: */
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *) addr;
        int i;
        for (i = 0; i < 16; i++) {
            if (addr6->sin6_addr.s6_addr[i] != 0) {
                return 0;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

void zRPC_inetaddr_make_wildcards(const zRPC_inetaddr *address, zRPC_inetaddr *wild4_out, zRPC_inetaddr *wild6_out) {
    zRPC_inetaddr_make_wildcard_v4(address, wild4_out);
    zRPC_inetaddr_make_wildcard_v6(address, wild6_out);
}


static uint16_t zRPC_inetaddr_get_port_origin(const zRPC_inetaddr *address) {
    const struct sockaddr *addr = (const struct sockaddr *) address->addr;
    switch (addr->sa_family) {
        case AF_INET:
            return ((struct sockaddr_in *) addr)->sin_port;
        case AF_INET6:
            return ((struct sockaddr_in6 *) addr)->sin6_port;
        default:
            return 0;
    }
}


void zRPC_inetaddr_make_wildcard_v4(const zRPC_inetaddr *address,
                                    zRPC_inetaddr *resolved_wild_out) {
    struct sockaddr_in *wild_out = (struct sockaddr_in *) resolved_wild_out->addr;
    memset(resolved_wild_out, 0, sizeof(*resolved_wild_out));
    wild_out->sin_family = AF_INET;
    wild_out->sin_port = zRPC_inetaddr_get_port_origin(address);
    resolved_wild_out->len = sizeof(struct sockaddr_in);
}

void zRPC_inetaddr_make_wildcard_v6(const zRPC_inetaddr *address,
                                    zRPC_inetaddr *resolved_wild_out) {
    struct sockaddr_in6 *wild_out =
            (struct sockaddr_in6 *) resolved_wild_out->addr;
    memset(resolved_wild_out, 0, sizeof(*resolved_wild_out));
    wild_out->sin6_family = AF_INET6;
    wild_out->sin6_port = zRPC_inetaddr_get_port_origin(address);
    resolved_wild_out->len = sizeof(struct sockaddr_in6);
}

static int host_join_port(char **out, const char *host, int port) {
    *out = malloc(256);
    if (host[0] != '[' && strchr(host, ':') != NULL) {
        /* IPv6 literals must be enclosed in brackets. */
        return sprintf(*out, "[%s]:%d", host, port);
    } else {
        /* Ordinary non-bracketed host:port. */
        return sprintf(*out, "%s:%d", host, port);
    }
}

int zRPC_inetaddr_to_string(char **out,
                            const zRPC_inetaddr *address,
                            int normalize) {
    const struct sockaddr *addr;
    const int save_errno = errno;
    zRPC_inetaddr addr_normalized;
    char ntop_buf[INET6_ADDRSTRLEN];
    const void *ip = NULL;
    int port;
    int ret;

    *out = NULL;
    if (normalize && zRPC_inetaddr_is_v4_mapped(address)) {
        zRPC_inetaddr_to_v4_mapped(address, &addr_normalized);
        address = &addr_normalized;
    }
    addr = (const struct sockaddr *) address->addr;
    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr4 = (const struct sockaddr_in *) addr;
        ip = &addr4->sin_addr;
        port = ntohs(addr4->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *) addr;
        ip = &addr6->sin6_addr;
        port = ntohs(addr6->sin6_port);
    }
    if (ip != NULL &&
        inet_ntop(addr->sa_family, ip, ntop_buf, sizeof(ntop_buf)) != NULL) {
        ret = host_join_port(out, ntop_buf, port);
    } else {
        *out = malloc(30);
        ret = sprintf(*out, "(sockaddr family=%d)", addr->sa_family);
    }
    /* This is probably redundant, but we wouldn't want to log the wrong error. */
    errno = save_errno;
    return ret;
}

char *zRPC_inetaddr_to_uri(const zRPC_inetaddr *address) {
    char *temp;
    char *result;
    zRPC_inetaddr addr_normalized;
    const struct sockaddr *addr;

    zRPC_inetaddr addr4_normalized;
    if (zRPC_inetaddr_is_v4_mapped(address)) {
        zRPC_inetaddr_to_v4_mapped(address, &addr4_normalized);
        address = &addr4_normalized;
    }

    addr = (const struct sockaddr *) address->addr;

    switch (addr->sa_family) {
        case AF_INET:
            zRPC_inetaddr_to_string(&temp, address, 0);
            result = (char *) malloc(128);
            sprintf(result, "ipv4:%s", temp);
            free(temp);
            return result;
        case AF_INET6:
            zRPC_inetaddr_to_string(&temp, address, 0);
            result = (char *) malloc(128);
            sprintf(result, "ipv6:%s", temp);
            free(temp);
            return result;
        default:
            return NULL;
    }
}

int zRPC_inetaddr_get_port(const zRPC_inetaddr *address) {
    return zRPC_inetaddr_get_port_origin(address);
}

int zRPC_inetaddr_set_port(const zRPC_inetaddr *address,
                           int port) {
    const struct sockaddr *addr = (const struct sockaddr *) address->addr;
    switch (addr->sa_family) {
        case AF_INET:
            ((struct sockaddr_in *) addr)->sin_port = htons((uint16_t) port);
            return 1;
        case AF_INET6:
            ((struct sockaddr_in6 *) addr)->sin6_port = htons((uint16_t) port);
            return 1;
        default:
            return 0;
    }
}


int parse_ipv4(const char *addr, zRPC_inetaddr *out) {
    const char *host_port = addr;
    char *host;
    char *port;
    int port_num;
    int result = 0;
    struct sockaddr_in *in = (struct sockaddr_in *) out->addr;

    if (*host_port == '/') ++host_port;
    if (!zRPC_split_host_port(host_port, &host, &port)) {
        return 0;
    }

    memset(out, 0, sizeof(zRPC_inetaddr));
    out->len = sizeof(struct sockaddr_in);
    in->sin_family = AF_INET;
    if (inet_pton(AF_INET, host, &in->sin_addr) == 0) {
        goto done;
    }

    if (port != NULL) {
        if (sscanf(port, "%d", &port_num) != 1 || port_num < 0 ||
            port_num > 65535) {
            goto done;
        }
        in->sin_port = htons((uint16_t) port_num);
    } else {
        goto done;
    }

    result = 1;
    done:
    free(host);
    free(port);
    return result;
}