//
// Created by zhsyourai on 12/1/16.
//

#ifndef ZRPC_RING_BUF_H
#define ZRPC_RING_BUF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef struct zRPC_ring_buffer zRPC_ring_buffer;

void zRPC_ring_buf_create(zRPC_ring_buffer **out, size_t capacity);

void zRPC_ring_buf_destroy(zRPC_ring_buffer *buf);

size_t zRPC_ring_buf_write(zRPC_ring_buffer *buf, const char *data, size_t bytes);

size_t zRPC_ring_buf_read(zRPC_ring_buffer *buf, char *data, size_t bytes);

size_t zRPC_ring_buf_can_write(zRPC_ring_buffer *buf);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_RING_BUF_H
