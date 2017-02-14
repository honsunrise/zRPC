//
// Created by zhsyourai on 11/27/16.
//

#include <memory.h>
#include "zRPC/rpc/callee.h"
#include "zRPC/support/rtti.h"
#include "zRPC/rpc/caller.h"
#include "zRPC/filter/msgpack_filter.h"
#include "zRPC/filter/litepackage_filter.h"

struct zRPC_caller {
    zRPC_filter_factory **filters;
    unsigned int filter_count;
    unsigned int filter_cap;
    zRPC_mutex mutex;
    zRPC_cond cond;
    zRPC_call_result *result;
    zRPC_queue *call_queue;
    zRPC_function_table_item *function_table;
    unsigned int function_count;
};

struct zRPC_caller_filter_custom_data {
    zRPC_caller *caller;
    zRPC_caller_instance *caller_instance;
};

static void
caller_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
    zRPC_caller_instance *caller_instance = malloc(sizeof(zRPC_caller_instance));
    caller_instance->channel = channel;
    zRPC_channel_set_custom_data(channel, caller_instance);
}

static void
caller_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    struct zRPC_caller_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    IF_TYPE_SAME(zRPC_call_result, msg) {
        zRPC_call_result *result = msg;
        zRPC_caller *caller = custom_data->caller;
        caller->result = result;
        zRPC_cond_notify_one(&caller->cond);
    } ELSE_IF_TYPE_SAME (zRPC_call, msg) {
        zRPC_call *call = msg;
        zRPC_caller *caller = custom_data->caller;
        const char *name;
        zRPC_call_get_function(call, &name);
        zRPC_call_result *result;
        zRPC_call_result_create(&result);
        for (int i = 0; i < caller->function_count; ++i) {
            if (strcmp(caller->function_table[i].name, name) == 0) {
                (caller->function_table[i].function_addr)(caller->function_table[i].param,
                                                          custom_data->caller_instance, call, result);
                break;
            }
        }
        zRPC_channel_write(channel, result);
        zRPC_call_result_destroy(result);
        zRPC_call_destroy(call);
    }
}

static void
caller_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_filter_out_add_item(out, msg);
}

static void
caller_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
    zRPC_caller_instance *caller_instance = malloc(sizeof(zRPC_caller_instance));
    free(caller_instance);
}

static zRPC_filter *zRPC_caller_filter_create(void *factory_custom) {
    zRPC_filter *filter;
    struct zRPC_caller_filter_custom_data *custom_data = malloc(sizeof(struct zRPC_caller_filter_custom_data));
    custom_data->caller = factory_custom;
    zRPC_filter_create(&filter, custom_data);

    zRPC_filter_set_on_active_callback(filter, caller_filter_on_active);
    zRPC_filter_set_on_read_callback(filter, caller_filter_on_readable);
    zRPC_filter_set_on_write_callback(filter, caller_filter_on_writable);
    zRPC_filter_set_on_inactive_callback(filter, caller_filter_on_inactive);
    return filter;
}


void zRPC_caller_create(zRPC_caller **out) {
    zRPC_caller *caller = malloc(sizeof(zRPC_caller));
    caller->filter_count = 0;
    caller->filter_cap = 10;
    caller->filters = malloc(sizeof(*caller->filters) * caller->filter_cap);
    zRPC_queue_create(&caller->call_queue);
    zRPC_mutex_init(&caller->mutex);
    zRPC_cond_init(&caller->cond);

    /*Init filters*/
    zRPC_filter_factory *filter1 = litepackage_filter_factory();
    zRPC_filter_factory *filter2 = msgpack_filter_factory();
    zRPC_filter_factory *filter3 = zRPC_filter_factory_create(zRPC_caller_filter_create, caller);

    zRPC_caller_add_filter(caller, filter1);
    zRPC_caller_add_filter(caller, filter2);
    zRPC_caller_add_filter(caller, filter3);

    *out = caller;
}

void zRPC_caller_destroy(zRPC_caller *caller) {
    if (caller) {
        free(caller);
    }
}

void zRPC_caller_get_filters(zRPC_caller *caller, zRPC_filter_factory ***filters, int *count) {
    *filters = caller->filters;
    *count = caller->filter_count;
}

void zRPC_caller_add_filter(zRPC_caller *caller, zRPC_filter_factory *filter) {
    if (caller->filter_count == caller->filter_cap - 1) {
        caller->filter_cap *= 2;
        caller->filters = realloc(caller->filters, sizeof(*caller->filters) * caller->filter_cap);
    }
    caller->filters[caller->filter_count++] = filter;
}

zRPC_call *
zRPC_caller_do_call(zRPC_caller *caller, zRPC_client *client, const char *name, zRPC_call_param *params, int count) {
    zRPC_call *call;
    zRPC_call_create(&call);
    zRPC_call_set_function(call, name);
    for (int i = 0; i < count; ++i) {
        zRPC_call_set_param(call, params[i].name, PASS_PTR(params[i].value, zRPC_value));
    }
    zRPC_client_write(client, call);
    return call;
}

void zRPC_caller_wait_result(zRPC_caller *caller, zRPC_call *call, zRPC_call_result **result) {
    zRPC_cond_wait(&caller->cond, &caller->mutex);
    *result = caller->result;
}

void zRPC_caller_destroy_result(zRPC_caller *caller, zRPC_call_result *result) {
    zRPC_call_result_destroy(result);
}

void zRPC_caller_set_function_callback_table(zRPC_caller *callee, zRPC_function_table_item *function_table,
                                             unsigned int count) {
    callee->function_table = function_table;
    callee->function_count = count;
}