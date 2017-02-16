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

    zRPC_call_stub_add_filter(call_stub, filter1);
    zRPC_call_stub_add_filter(call_stub, filter2);
    zRPC_call_stub_add_filter(call_stub, filter3);

    *out = call_stub;
}

void zRPC_call_stub_destroy(zRPC_call_stub *caller) {
    if (caller) {
        free(caller->filters);
        free(caller);
    }
}

void zRPC_call_stub_get_filters(zRPC_call_stub *call_stub, zRPC_filter_factory ***filters, int *count) {
    *filters = call_stub->filters;
    *count = call_stub->filter_count;
}

void zRPC_call_stub_add_filter(zRPC_call_stub *caller, zRPC_filter_factory *filter) {
    if (caller->filter_count == caller->filter_cap - 1) {
        caller->filter_cap *= 2;
        caller->filters = realloc(caller->filters, sizeof(*caller->filters) * caller->filter_cap);
    }
    caller->filters[caller->filter_count++] = filter;
}