//
// Created by zhsyourai on 11/27/16.
//

#ifndef ZRPC_CALLEE_H
#define ZRPC_CALLEE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/support/runnable.h"
#include "zRPC/filter.h"
#include "call.h"

typedef struct zRPC_callee zRPC_callee;

void zRPC_callee_create(zRPC_callee **out, void(*caller_con_callback)(zRPC_caller_instance *));

void zRPC_callee_destroy(zRPC_callee *callee);

void zRPC_callee_get_filters(zRPC_callee *callee, zRPC_filter ***filters, int *count);

void zRPC_callee_add_filter(zRPC_callee *callee, zRPC_filter *filter);

void zRPC_callee_do_call_callback(zRPC_caller_instance *caller, const char *name, zRPC_call_param *params, int count);

void zRPC_callee_wait_result(zRPC_caller_instance *caller, zRPC_call_result **result);

void zRPC_callee_destroy_result(zRPC_caller_instance *caller, zRPC_call_result *result);

void zRPC_callee_set_function_table(zRPC_callee *callee, zRPC_function_table_item *function_table, unsigned int count);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CALLEE_H
