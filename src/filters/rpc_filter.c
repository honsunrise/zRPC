//
// Created by zhsyourai on 2/16/17.
//

#include <malloc.h>
#include "zRPC/rpc/call.h"
#include "zRPC/ds/hashmap.h"
#include "zRPC/support/rtti.h"
#include "zRPC/filter.h"
#include "zRPC/filter/rpc_filter.h"

struct zRPC_call_stub_filter_custom_data {
    zRPC_mutex mutex;
    zRPC_hashmap *calling_map;
    zRPC_caller_instance *caller_instance;
    call_function function;
    void *param;
};

void rpc_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
    struct zRPC_call_stub_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    custom_data->caller_instance = malloc(sizeof(zRPC_caller_instance));
    custom_data->caller_instance->channel = channel;
}

void rpc_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    struct zRPC_call_stub_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    IF_TYPE_SAME(zRPC_call_result, msg) {
        zRPC_call_result *result = msg;
        zRPC_mutex_lock(&custom_data->mutex);
        zRPC_call *call = hashmapRemove(custom_data->calling_map, (void *) result->request_id);
        if (call != NULL) {
            call->result = result;
            zRPC_sem_post(&call->sem);
        }
        zRPC_mutex_unlock(&custom_data->mutex);
    } ELSE_IF_TYPE_SAME (zRPC_call, msg) {
        zRPC_call *call = msg;
        const char *name;
        zRPC_call_get_function(call, &name);
        zRPC_call_result *result;
        zRPC_call_result_create(&result);
        custom_data->function(custom_data->param, name, custom_data->caller_instance, call, result);
        zRPC_channel_write(channel, result);
        zRPC_call_result_destroy(result);
        zRPC_call_destroy(call);
    }
}

void rpc_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    struct zRPC_call_stub_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    zRPC_call *call = msg;
    zRPC_mutex_lock(&custom_data->mutex);
    hashmapPut(custom_data->calling_map, (void *) call->request_id, call);
    zRPC_mutex_unlock(&custom_data->mutex);
    zRPC_filter_out_add_item(out, call);
}

void rpc_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
    struct zRPC_call_stub_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    free(custom_data->caller_instance);
}

static int int_hash_function(void *key) {
    return (int)(key);
}

static int equals(void* keyA, void* keyB) {
    return keyA == keyB;
}

struct rpc_filter_factory_param {
    call_function function;
    void *param;
};

static zRPC_filter *rpc_filter_create(void *factory_custom) {
    zRPC_filter *filter;
    struct zRPC_call_stub_filter_custom_data *custom_data = malloc(sizeof(struct zRPC_call_stub_filter_custom_data));
    struct rpc_filter_factory_param *param = factory_custom;
    zRPC_mutex_init(&custom_data->mutex);
    custom_data->calling_map = hashmapCreate(1024, int_hash_function, equals);
    custom_data->function = param->function;
    custom_data->param = param->param;
    zRPC_filter_create(&filter, custom_data);

    zRPC_filter_set_on_active_callback(filter, rpc_filter_on_active);
    zRPC_filter_set_on_read_callback(filter, rpc_filter_on_readable);
    zRPC_filter_set_on_write_callback(filter, rpc_filter_on_writable);
    zRPC_filter_set_on_inactive_callback(filter, rpc_filter_on_inactive);
    return filter;
}

zRPC_filter_factory *rpc_filter_factory_instance = NULL;

zRPC_filter_factory *rpc_filter_factory(call_function function, void *param) {
    if (rpc_filter_factory_instance == NULL) {
        struct rpc_filter_factory_param *factory_param = malloc(sizeof(struct rpc_filter_factory_param));
        factory_param->function = function;
        factory_param->param = param;
        rpc_filter_factory_instance = zRPC_filter_factory_create(rpc_filter_create, factory_param);
    }
    return rpc_filter_factory_instance;
}