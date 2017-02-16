//
// Created by zhsyourai on 2/16/17.
//

#ifndef ZRPC_CALL_STUB_H
#define ZRPC_CALL_STUB_H
#ifdef __cplusplus
extern "C" {
#endif

#include "call.h"
#include "zRPC/filter.h"
#include "zRPC/client.h"

typedef struct zRPC_call_stub zRPC_call_stub;

void zRPC_call_stub_create(zRPC_call_stub **out, zRPC_function_table_item *function_table, unsigned int count);

void zRPC_call_stub_destroy(zRPC_call_stub *caller);

void zRPC_call_stub_get_filters(zRPC_call_stub *call_stub, zRPC_filter_factory ***filters, int *count);

void zRPC_call_stub_add_filter(zRPC_call_stub *call_stub, zRPC_filter_factory *filter);
#ifdef __cplusplus
}
#endif
#endif //ZRPC_CALL_STUB_H
