//
// Created by zhsyourai on 12/1/16.
//

#include <malloc.h>
#include <memory.h>
#include "zRPC/ds/ring_buf.h"
#include "zRPC/support/useful.h"

struct zRPC_ring_buffer {
    size_t front;
    size_t rear;
    size_t size;
    size_t capacity;
    char *data;
};

void zRPC_ring_buf_create(zRPC_ring_buffer **out, size_t capacity) {
    zRPC_ring_buffer *buf = malloc(sizeof(zRPC_ring_buffer));
    buf->capacity = capacity;
    buf->front = 0;
    buf->rear = 0;
    buf->size = 0;
    buf->data = malloc(capacity);
    *out = buf;
}

void zRPC_ring_buf_destroy(zRPC_ring_buffer *buf) {
    if (buf != NULL) {
        free(buf->data);
        free(buf);
    }
}

size_t zRPC_ring_buf_write(zRPC_ring_buffer *buf, const char *data, size_t bytes) {
    if (bytes == 0)
        return 0;

    size_t capacity = buf->capacity;
    size_t bytes_to_write = MIN(bytes, capacity - buf->size);

    // Write in a single step
    if (bytes_to_write <= capacity - buf->rear) {
        memcpy(buf->data + buf->rear, data, bytes_to_write);
        buf->rear += bytes_to_write;
        if (buf->rear == capacity) buf->rear = 0;
    }
        // Write in two steps
    else {
        size_t size_1 = capacity - buf->rear;
        memcpy(buf->data + buf->rear, data, size_1);
        size_t size_2 = bytes_to_write - size_1;
        memcpy(buf->data, data + size_1, size_2);
        buf->rear = size_2;
    }

    buf->size += bytes_to_write;
    return bytes_to_write;
}

size_t zRPC_ring_buf_read(zRPC_ring_buffer *buf, char *data, size_t bytes) {
    if (bytes == 0) return 0;

    size_t capacity = buf->capacity;
    size_t bytes_to_read = MIN(bytes, buf->size);

    // Read in a single step
    if (bytes_to_read <= capacity - buf->front) {
        memcpy(data, buf->data + buf->front, bytes_to_read);
        buf->front += bytes_to_read;
        if (buf->front == capacity) buf->front = 0;
    }
        // Read in two steps
    else {
        size_t size_1 = capacity - buf->front;
        memcpy(data, buf->data + buf->front, size_1);
        size_t size_2 = bytes_to_read - size_1;
        memcpy(data + size_1, buf->data, size_2);
        buf->front = size_2;
    }

    buf->size -= bytes_to_read;
    return bytes_to_read;
}

size_t zRPC_ring_buf_can_write(zRPC_ring_buffer *buf) {
    size_t capacity = buf->capacity;
    return capacity - buf->size;
}
