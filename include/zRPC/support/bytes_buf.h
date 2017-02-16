//
// Created by zhsyourai on 11/27/16.
//

#ifndef ZRPC_BYTES_BUF_H
#define ZRPC_BYTES_BUF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "ref_count.h"

typedef struct zRPC_bytes_buf zRPC_bytes_buf;

DECLARE_REFERENCE_FUN(zRPC_bytes_buf)

void zRPC_bytes_buf_create(size_t size, zRPC_bytes_buf **out);

void zRPC_bytes_buf_warp(void *addr, size_t size, zRPC_bytes_buf **out);

void *zRPC_bytes_buf_addr(zRPC_bytes_buf *buf);

size_t zRPC_bytes_buf_len(zRPC_bytes_buf *buf);

void zRPC_bytes_buf_destroy(zRPC_bytes_buf *buf);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_BYTES_BUF_H
