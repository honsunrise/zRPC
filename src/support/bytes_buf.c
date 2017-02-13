//
// Created by zhsyourai on 11/27/16.
//
#include <malloc.h>

#include "zRPC/support/bytes_buf.h"

struct zRPC_bytes_buf {
    DECLARE_REFERENCE;
    void *addr;
    size_t len;
};

static void release_function(void *ptr) {
    zRPC_bytes_buf *buf = ptr;
    if (buf) {
        free(buf->addr);
        free(buf);
    }
}

DEFINE_REFERENCE_FUN(zRPC_bytes_buf)

void zRPC_bytes_buf_create(size_t size, zRPC_bytes_buf **out) {
    zRPC_bytes_buf *buf = malloc(sizeof(zRPC_bytes_buf));
    INIT_REFERENCE(buf, release_function);
    buf->addr = malloc(size);
    buf->len = size;
    *out = buf;
}

void zRPC_bytes_buf_warp(void *addr, size_t size, zRPC_bytes_buf **out) {
    zRPC_bytes_buf *buf = malloc(sizeof(zRPC_bytes_buf));
    INIT_REFERENCE(buf, release_function);
    buf->addr = addr;
    buf->len = size;
    *out = buf;
}

void *zRPC_bytes_buf_addr(zRPC_bytes_buf *buf) {
    return buf->addr;
}

size_t zRPC_bytes_buf_len(zRPC_bytes_buf *buf) {
    return buf->len;
}


void zRPC_bytes_buf_destroy(zRPC_bytes_buf *buf) {
    release_function(buf);
}
