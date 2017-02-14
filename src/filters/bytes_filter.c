//
// Created by zhsyourai on 12/13/16.
//

#include <string.h>
#include "zRPC/filter.h"
#include "zRPC/support/ring_buf.h"
#include "zRPC/support/bytes_buf.h"
#include "zRPC/filter/bytes_filter.h"

typedef struct bytes_filter_data {
    size_t cap;
} bytes_filter_data;

void
bytes_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
}

void
bytes_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    bytes_filter_data *custom = zRPC_filter_get_custom_data(filter);
    zRPC_ring_buffer *ring_buf = msg;
    char *buf = malloc(custom->cap);
    size_t len = 0, need_read, read = 0;
    do {
        if (len >= custom->cap) {
            custom->cap *= 2;
            buf = realloc(buf, custom->cap);
        }
        need_read = custom->cap - len;
        read = zRPC_ring_buf_read(ring_buf, buf + len, need_read);
        len += read;
    } while (read == need_read);
    zRPC_bytes_buf *buf_out;
    zRPC_bytes_buf_warp(buf, (unsigned int) len, &buf_out);
    zRPC_filter_out_add_item(out, PASS_PTR(buf_out, zRPC_bytes_buf));
}

void
bytes_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_bytes_buf *buf = msg;
    zRPC_filter_out_add_item(out, PASS_PTR(buf, zRPC_bytes_buf));
    SUB_REFERENCE(buf, zRPC_bytes_buf);
}

void
bytes_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
}

static zRPC_filter *bytes_filter_create(void *factory_custom) {
    zRPC_filter *filter;
    bytes_filter_data *custom = malloc(sizeof(bytes_filter_data));
    custom->cap = 512;
    zRPC_filter_create(&filter, custom);
    zRPC_filter_set_on_active_callback(filter, bytes_filter_on_active);
    zRPC_filter_set_on_read_callback(filter, bytes_filter_on_readable);
    zRPC_filter_set_on_write_callback(filter, bytes_filter_on_writable);
    zRPC_filter_set_on_inactive_callback(filter, bytes_filter_on_inactive);
    return filter;
}

zRPC_filter_factory *bytes_filter_factory_instance = NULL;

zRPC_filter_factory *bytes_filter_factory() {
    if(bytes_filter_factory_instance == NULL) {
        bytes_filter_factory_instance = zRPC_filter_factory_create(bytes_filter_create, NULL);
    }
    return bytes_filter_factory_instance;
}