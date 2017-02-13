//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_FILTER_H
#define ZRPC_FILTER_H
#ifdef __cplusplus
extern "C" {
#endif

struct zRPC_channel;

typedef struct zRPC_filter zRPC_filter;

typedef zRPC_filter* (*filter_factory_create)(void *custom_data);

typedef struct zRPC_filter_factory zRPC_filter_factory;

typedef struct zRPC_filter_out zRPC_filter_out;

typedef void (*filter_callback)(zRPC_filter *filter, struct zRPC_channel *, void *tag);

typedef void (*filter_callback_data)(zRPC_filter *filter, struct zRPC_channel *, void *msg, zRPC_filter_out *out,
                                     void *tag);

zRPC_filter_factory *zRPC_filter_factory_create(filter_factory_create factory, void *custom);

void zRPC_filter_create_by_factory(zRPC_filter **out, zRPC_filter_factory *factory);

void zRPC_filter_create(zRPC_filter **out, void *custom_data);

void zRPC_filter_destroy(zRPC_filter *filter);

void zRPC_filter_set_on_active_callback(zRPC_filter *filter, filter_callback callback, void *tag);

void zRPC_filter_set_on_inactive_callback(zRPC_filter *filter, filter_callback callback, void *tag);

void zRPC_filter_set_on_read_callback(zRPC_filter *filter, filter_callback_data callback, void *tag);

void zRPC_filter_set_on_write_callback(zRPC_filter *filter, filter_callback_data callback, void *tag);

void zRPC_filter_set_custom_data(zRPC_filter *filter, void *custom_data);

void *zRPC_filter_get_custom_data(zRPC_filter *filter);

void zRPC_filter_out_create(zRPC_filter_out **out);

void zRPC_filter_out_destroy(zRPC_filter_out *out);

int zRPC_filter_out_item_count(zRPC_filter_out *out);

void *zRPC_filter_out_get_item(zRPC_filter_out *out, unsigned int i);

void zRPC_filter_out_add_item(zRPC_filter_out *out, void *item);

void zRPC_filter_out_remove_item(zRPC_filter_out *out, void *item);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_FILTER_H
