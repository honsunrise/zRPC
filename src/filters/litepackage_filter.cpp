//
// Created by zhsyourai on 12/1/16.
//

#include <memory.h>
#include "zRPC/filter.h"
#include "zRPC/filter/litepackage_filter.h"
#include "zRPC/ds/ring_buf.h"
#include "zRPC/support/bytes_buf.h"

typedef struct lite_package_header {
    int32_t len;
} lite_package_header;

typedef struct lite_package {
    lite_package_header header;
    zRPC_bytes_buf *body;
} lite_package;

struct zRPC_litepackage_custom {
    int new_package;
    size_t header_remainder, header_pos;
    size_t remainder, pos;
    lite_package package;
};

void
litepackage_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
}

void
litepackage_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
}

void
litepackage_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_ring_buffer *buf = (zRPC_ring_buffer *) msg;
    zRPC_litepackage_custom *custom = (zRPC_litepackage_custom *) zRPC_filter_get_custom_data(filter);
    lite_package *package = &custom->package;
    size_t read;
    while (1) {
        if (custom->new_package) {
            read = zRPC_ring_buf_read(buf, (char *) &package->header + custom->header_pos, custom->header_remainder);
            if (read == 0)
                return;
            if (read < custom->header_remainder) {
                custom->header_remainder -= read;
                custom->header_pos += read;
                return;
            }
            if (package->header.len == 0) {
                return;
            }
            zRPC_bytes_buf_create((size_t) package->header.len, &package->body);
            custom->remainder = (size_t) package->header.len;
            custom->pos = 0;
            custom->new_package = 0;
        }

        read = zRPC_ring_buf_read(buf, (char *) zRPC_bytes_buf_addr(package->body) + custom->pos, custom->remainder);
        if (read == 0)
            return;
        if (read == custom->remainder) {
            zRPC_filter_out_add_item(out, PASS_PTR(package->body, zRPC_bytes_buf));
            SUB_REFERENCE(package->body, zRPC_bytes_buf);
            package->body = NULL;
            custom->new_package = 1;
            custom->header_remainder = sizeof(lite_package_header);
            custom->header_pos = 0;
        } else {
            custom->remainder -= read;
            custom->pos += read;
        }
    }
}

void
litepackage_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_bytes_buf *buf = (zRPC_bytes_buf *) msg;
    zRPC_bytes_buf *buf_out;
    size_t header_len = sizeof(lite_package_header);
    zRPC_bytes_buf_create(header_len + zRPC_bytes_buf_len(buf), &buf_out);
    lite_package_header package_header;
    package_header.len = (int32_t) zRPC_bytes_buf_len(buf);
    memcpy(zRPC_bytes_buf_addr(buf_out), &package_header, header_len);
    memcpy((char *) zRPC_bytes_buf_addr(buf_out) + header_len, zRPC_bytes_buf_addr(buf), zRPC_bytes_buf_len(buf));
    zRPC_filter_out_add_item(out, PASS_PTR(buf_out, zRPC_bytes_buf));
    SUB_REFERENCE(buf, zRPC_bytes_buf);
    SUB_REFERENCE(buf_out, zRPC_bytes_buf);
}

void
litepackage_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
}

static zRPC_filter *litepackage_filter_create(void *fatory_custom) {
    zRPC_filter *filter;
    zRPC_litepackage_custom *custom = (zRPC_litepackage_custom *) malloc(sizeof(zRPC_litepackage_custom));
    custom->new_package = 1;
    custom->remainder = 0;
    custom->pos = 0;
    custom->header_remainder = sizeof(lite_package_header);
    custom->header_pos = 0;
    zRPC_filter_create(&filter, custom);
    zRPC_filter_set_on_active_callback(filter, litepackage_filter_on_active);
    zRPC_filter_set_on_read_callback(filter, litepackage_filter_on_readable);
    zRPC_filter_set_on_write_callback(filter, litepackage_filter_on_writable);
    zRPC_filter_set_on_inactive_callback(filter, litepackage_filter_on_inactive);
    return filter;
}

zRPC_filter_factory *litepackage_filter_factory_instance = NULL;

zRPC_filter_factory *litepackage_filter_factory() {
    if(litepackage_filter_factory_instance == NULL) {
        litepackage_filter_factory_instance = zRPC_filter_factory_create(litepackage_filter_create, NULL);
    }
    return litepackage_filter_factory_instance;
}