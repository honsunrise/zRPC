//
// Created by zhsyourai on 11/26/16.
//

#ifndef ZRPC_CALL_H
#define ZRPC_CALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "../../../src/include/rtti.h"
#include "zRPC/channel.h"
#include "zRPC/support/lock.h"
#include "zRPC/support/var_type.h"

typedef struct zRPC_callback_desc zRPC_callback_desc;

typedef struct zRPC_call_param {
    const char *name;
    zRPC_value *value;
} zRPC_call_param;

typedef struct zRPC_call_result {
    DECLARE_RTTI(zRPC_call_result);
    int request_id;
    unsigned int result_count;
    unsigned int result_cab;
    zRPC_call_param *results;
} zRPC_call_result;

typedef struct zRPC_call {
    DECLARE_RTTI(zRPC_call);
    int request_id;
    const char *name;
    int flag;
    unsigned int param_count;
    unsigned int param_cab;
    zRPC_call_param *params;
    zRPC_call_result *result;
    zRPC_sem sem;
} zRPC_call;

typedef struct zRPC_caller_instance {
    zRPC_channel *channel;
} zRPC_caller_instance;

typedef struct zRPC_function_table_item {
    const char *name;
    void *param;

    void
    (*function_addr)(void *param, zRPC_caller_instance *caller_instance, zRPC_call *call, zRPC_call_result *result);
} zRPC_function_table_item;

void zRPC_call_create(zRPC_call **out);

void zRPC_call_destroy(zRPC_call *call);

void zRPC_call_set_function(zRPC_call *call, const char *name);

void zRPC_call_set_param(zRPC_call *call, const char *name, zRPC_value *value);

void zRPC_call_get_function(zRPC_call *call, const char **name);

void zRPC_call_get_param(zRPC_call *call, const char *name, zRPC_value **value);

void zRPC_call_get_params(zRPC_call *call, zRPC_call_param **params, unsigned int *count);

void zRPC_call_result_create(zRPC_call_result **out);

void zRPC_call_result_destroy(zRPC_call_result *result);

void zRPC_call_result_set_result(zRPC_call_result *result, const char *name, zRPC_value *value);

void zRPC_call_result_get_param(zRPC_call_result *result, const char *name, zRPC_value **value);

void zRPC_call_result_get_results(zRPC_call_result *result, zRPC_call_param **results, unsigned int *count);

zRPC_call *zRPC_call_do_call(zRPC_channel *channel, const char *name, zRPC_call_param *params, int count);

void zRPC_call_wait_result(zRPC_call *call, zRPC_call_result **result);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CALL_H
