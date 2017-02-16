//
// Created by zhsyourai on 2/16/17.
//

#include <memory.h>
#include "zRPC/filter/rpc_filter.h"
#include "zRPC/rpc/call_stub.h"
#include "zRPC/filter/msgpack_filter.h"
#include "zRPC/filter/litepackage_filter.h"
#include "zRPC/ds/hashmap.h"

struct zRPC_call_stub {
    zRPC_filter_factory **filters;
    unsigned int filter_count;
    unsigned int filter_cap;
    zRPC_mutex mutex;
    zRPC_function_table_item *function_table;
    unsigned int function_count;
    zRPC_channel *channel;
};

static void function(void *param, const char*name,
              zRPC_caller_instance *caller_instance, zRPC_call *call, zRPC_call_result *result) {
    zRPC_call_stub *call_stub = param;
    for (int i = 0; i < call_stub->function_count; ++i) {
        if (strcmp(call_stub->function_table[i].name, name) == 0) {
            (call_stub->function_table[i].function_addr)
                    (call_stub->function_table[i].param, caller_instance, call, result);
            break;
        }
    }
}

static void call_stub_filter_on_active(zRPC_filter *filter, zRPC_channel *channel) {
    zRPC_call_stub *call_stub = *(zRPC_call_stub **)zRPC_filter_get_custom_data(filter);
    call_stub->channel = channel;
}

static void call_stub_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_filter_out_add_item(out, msg);
}

static void call_stub_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out) {
    zRPC_filter_out_add_item(out, msg);
}

static void call_stub_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel) {
    zRPC_call_stub *call_stub = *(zRPC_call_stub **)zRPC_filter_get_custom_data(filter);
    call_stub->channel = NULL;
}

static zRPC_filter *call_stub_filter_create(void *factory_custom) {
    zRPC_filter *filter;
    zRPC_call_stub **factory_param = malloc(sizeof(uintptr_t));
    *factory_param = factory_custom;
    zRPC_filter_create(&filter, factory_param);
    zRPC_filter_set_on_active_callback(filter, call_stub_filter_on_active);
    zRPC_filter_set_on_read_callback(filter, call_stub_filter_on_readable);
    zRPC_filter_set_on_write_callback(filter, call_stub_filter_on_writable);
    zRPC_filter_set_on_inactive_callback(filter, call_stub_filter_on_inactive);
    return filter;
}

void zRPC_call_stub_create(zRPC_call_stub **out, zRPC_function_table_item *function_table, unsigned int count) {
    zRPC_call_stub *call_stub = malloc(sizeof(zRPC_call_stub));
    call_stub->filter_count = 0;
    call_stub->filter_cap = 10;
    call_stub->filters = malloc(sizeof(*call_stub->filters) * call_stub->filter_cap);
    call_stub->function_table = function_table;
    call_stub->function_count = count;
    zRPC_mutex_init(&call_stub->mutex);
    /*Init filters*/
    zRPC_filter_factory *filter1 = litepackage_filter_factory();
    zRPC_filter_factory *filter2 = msgpack_filter_factory();
    zRPC_filter_factory *filter3 = rpc_filter_factory(function, call_stub);
    zRPC_filter_factory *filter4 = zRPC_filter_factory_create(call_stub_filter_create, call_stub);

    zRPC_call_stub_add_filter(call_stub, filter1);
    zRPC_call_stub_add_filter(call_stub, filter2);
    zRPC_call_stub_add_filter(call_stub, filter3);
    zRPC_call_stub_add_filter(call_stub, filter4);

    *out = call_stub;
}

void zRPC_call_stub_destroy(zRPC_call_stub *caller) {
    if (caller) {
        free(caller);
    }
}

void zRPC_call_stub_get_filters(zRPC_call_stub *caller, zRPC_filter_factory ***filters, int *count) {
    *filters = caller->filters;
    *count = caller->filter_count;
}

void zRPC_call_stub_add_filter(zRPC_call_stub *caller, zRPC_filter_factory *filter) {
    if (caller->filter_count == caller->filter_cap - 1) {
        caller->filter_cap *= 2;
        caller->filters = realloc(caller->filters, sizeof(*caller->filters) * caller->filter_cap);
    }
    caller->filters[caller->filter_count++] = filter;
}

zRPC_call *
zRPC_call_stub_do_call(zRPC_call_stub *call_stub, const char *name, zRPC_call_param *params, int count) {
    static int request_id = 0;
    zRPC_call *call;
    zRPC_call_create(&call);
    call->request_id = request_id++;
    zRPC_call_set_function(call, name);
    for (int i = 0; i < count; ++i) {
        zRPC_call_set_param(call, params[i].name, PASS_PTR(params[i].value, zRPC_value));
    }
    zRPC_channel_write(call_stub->channel, call);
    return call;
}

void zRPC_call_stub_wait_result(zRPC_call *call, zRPC_call_result **result) {
    zRPC_call_result *ret = NULL;
    do {
        zRPC_sem_wait(&call->sem);
        ret = call->result;
    } while (ret == NULL);
    *result = ret;
}