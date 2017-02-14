//
// Created by zhsyourai on 11/27/16.
//

#include <malloc.h>
#include <memory.h>
#include "zRPC/support/rtti.h"
#include "zRPC/rpc/callee.h"
#include "zRPC/filter/litepackage_filter.h"
#include "zRPC/filter/msgpack_filter.h"

struct zRPC_callee {
    zRPC_filter_factory **filters;
    unsigned int filter_count;
    unsigned int filter_cap;
    zRPC_function_table_item *function_table;
    unsigned int function_count;

    void (*caller_con_callback)(zRPC_caller_instance *);
};

struct zRPC_callee_filter_custom_data {
    zRPC_callee *callee;
    zRPC_caller_instance *caller_instance;
};

static void callee_filter_on_active(zRPC_filter *filter, zRPC_channel *channel, void *tag) {
    struct zRPC_callee_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    zRPC_caller_instance *caller_instance = malloc(sizeof(zRPC_caller_instance));
    zRPC_callee *callee = custom_data->callee;
    custom_data->caller_instance = caller_instance;
    caller_instance->channel = channel;
    caller_instance->result = NULL;
    zRPC_cond_init(&caller_instance->cond);
    zRPC_channel_set_custom_data(channel, caller_instance);
    if (callee->caller_con_callback)
        callee->caller_con_callback(caller_instance);
}

static void
callee_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag) {
    struct zRPC_callee_filter_custom_data *custom_data = zRPC_filter_get_custom_data(filter);
    IF_TYPE_SAME(zRPC_call_result, msg) {
        zRPC_call_result *result = msg;
        zRPC_mutex_lock(&custom_data->caller_instance->mutex);
        custom_data->caller_instance->result = result;
        zRPC_mutex_unlock(&custom_data->caller_instance->mutex);
        zRPC_cond_notify_one(&custom_data->caller_instance->cond);
    } ELSE_IF_TYPE_SAME (zRPC_call, msg) {
        zRPC_call *call = msg;
        zRPC_callee *callee = custom_data->callee;
        const char *name;
        zRPC_call_get_function(call, &name);
        zRPC_call_result *result;
        zRPC_call_result_create(&result);
        for (int i = 0; i < callee->function_count; ++i) {
            if (strcmp(callee->function_table[i].name, name) == 0) {
                (callee->function_table[i].function_addr)(callee->function_table[i].param, custom_data->caller_instance, call,
                                                          result);
                break;
            }
        }
        zRPC_channel_write(channel, result);
        zRPC_call_result_destroy(result);
        zRPC_call_destroy(call);
    }
}

static void
callee_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag) {
    zRPC_filter_out_add_item(out, msg);
}


static void callee_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel, void *tag) {
}

static zRPC_filter *zRPC_callee_filter_create(void *factory_custom) {
    zRPC_filter *filter;
    struct zRPC_callee_filter_custom_data *custom_data = malloc(sizeof(struct zRPC_callee_filter_custom_data));
    custom_data->callee = factory_custom;
    zRPC_filter_create(&filter, custom_data);

    zRPC_filter_set_on_active_callback(filter, callee_filter_on_active, NULL);
    zRPC_filter_set_on_read_callback(filter, callee_filter_on_readable, NULL);
    zRPC_filter_set_on_write_callback(filter, callee_filter_on_writable, NULL);
    zRPC_filter_set_on_inactive_callback(filter, callee_filter_on_inactive, NULL);
    return filter;
}

void zRPC_callee_create(zRPC_callee **out, void(*caller_con_callback)(zRPC_caller_instance *)) {
    zRPC_callee *callee = malloc(sizeof(zRPC_callee));
    callee->filter_count = 0;
    callee->filter_cap = 10;
    callee->filters = malloc(sizeof(*callee->filters) * callee->filter_cap);
    callee->caller_con_callback = caller_con_callback;

    /*Init filters*/
    zRPC_filter_factory *filter1 = litepackage_filter_factory();
    zRPC_filter_factory *filter2 = msgpack_filter_factory();
    zRPC_filter_factory *filter3 = zRPC_filter_factory_create(zRPC_callee_filter_create, callee);

    zRPC_callee_add_filter(callee, filter1);
    zRPC_callee_add_filter(callee, filter2);
    zRPC_callee_add_filter(callee, filter3);

    *out = callee;
}

void zRPC_callee_destroy(zRPC_callee *callee) {
    if (callee) {
        free(callee);
    }
}

void zRPC_callee_get_filters(zRPC_callee *callee, zRPC_filter_factory ***filters, int *count) {
    *filters = callee->filters;
    *count = callee->filter_count;
}

void zRPC_callee_add_filter(zRPC_callee *callee, zRPC_filter_factory *filter) {
    if (callee->filter_count == callee->filter_cap - 1) {
        callee->filter_cap *= 2;
        callee->filters = realloc(callee->filters, sizeof(*callee->filters) * callee->filter_cap);
    }
    callee->filters[callee->filter_count++] = filter;
}

void zRPC_callee_do_call_callback(zRPC_caller_instance *caller, const char *name, zRPC_call_param *params, int count) {
    zRPC_call *call;
    zRPC_call_create(&call);
    zRPC_call_set_function(call, name);
    for (int i = 0; i < count; ++i) {
        zRPC_call_set_param(call, params[i].name, PASS_PTR(params[i].value, zRPC_value));
    }
    zRPC_channel_write(caller->channel, call);
    zRPC_call_destroy(call);
}

void zRPC_callee_wait_result(zRPC_caller_instance *caller, zRPC_call_result **result) {
    zRPC_mutex_lock(&caller->mutex);
    while (caller->result == NULL)
        zRPC_cond_wait(&caller->cond, &caller->mutex);
    *result = caller->result;
    caller->result = NULL;
    zRPC_mutex_unlock(&caller->mutex);
}

void zRPC_callee_destroy_result(zRPC_caller_instance *caller, zRPC_call_result *result) {
    zRPC_call_result_destroy(result);
}

void zRPC_callee_set_function_table(zRPC_callee *callee, zRPC_function_table_item *function_table, unsigned int count) {
    callee->function_table = function_table;
    callee->function_count = count;
}
