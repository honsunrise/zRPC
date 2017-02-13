//
// Created by zhsyourai on 11/27/16.
//

#ifndef ZRPC_CALLER_H
#define ZRPC_CALLER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "call.h"
#include "zRPC/filter.h"
#include "zRPC/client.h"

typedef struct zRPC_caller zRPC_caller;

void zRPC_caller_create(zRPC_caller **out);

void zRPC_caller_destroy(zRPC_caller *caller);

void zRPC_caller_get_filters(zRPC_caller *caller, zRPC_filter ***filters, int *count);

void zRPC_caller_add_filter(zRPC_caller *caller, zRPC_filter *filter);

zRPC_call *
zRPC_caller_do_call(zRPC_caller *caller, zRPC_client *client, const char *name, zRPC_call_param *params, int count);

void zRPC_caller_wait_result(zRPC_caller *caller, zRPC_call *call, zRPC_call_result **result);

void zRPC_caller_destroy_result(zRPC_caller *caller, zRPC_call_result *result);

void zRPC_caller_set_function_callback_table(zRPC_caller *callee, zRPC_function_table_item *function_table,
                                             unsigned int count);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CALLER_H
