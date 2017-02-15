//
// Created by zhsyourai on 11/26/16.
//

#include <malloc.h>
#include <memory.h>
#include "zRPC/rpc/call.h"
#include "zRPC/support/rtti.h"

struct zRPC_callback_desc {

};

void zRPC_call_create(zRPC_call **out) {
    zRPC_call *call = malloc(sizeof(zRPC_call));
    RTTI_INIT_PTR(zRPC_call, call);
    call->name = NULL;
    call->flag = 0;
    call->request_id = 0;
    call->param_cab = 5;
    call->params = malloc(sizeof(*call->params) * call->param_cab);
    call->param_count = 0;
    zRPC_sem_init(&call->sem, 0);
    *out = call;
}

void zRPC_call_destroy(zRPC_call *call) {
    if (call != NULL) {
        for (int i = 0; i < call->param_count; ++i) {
            free((void *) call->params[i].name);
            SUB_REFERENCE(call->params[i].value, zRPC_value);
        }
        free(call->params);
        free((void *) call->name);
        zRPC_sem_destroy(&call->sem);
        free(call);
    }
}

void zRPC_call_set_function(zRPC_call *call, const char *name) {
    call->name = malloc(strlen(name) + 1);
    bzero((void *) call->name, strlen(name) + 1);
    strcpy((char *) call->name, name);
}

void zRPC_call_set_param(zRPC_call *call, const char *name, zRPC_value *value) {
    if (call->param_count >= call->param_cab - 1) {
        call->param_cab *= 1.5;
        call->params = realloc(call->params, call->param_cab * sizeof(*call->params));
    }
    call->params[call->param_count].name = malloc(strlen(name) + 1);
    bzero((void *) call->params[call->param_count].name, strlen(name) + 1);
    strcpy((char *) call->params[call->param_count].name, name);
    call->params[call->param_count++].value = value;
    SUB_REFERENCE(value, zRPC_value);
}

void zRPC_call_get_function(zRPC_call *call, const char **name) {
    *name = call->name;
}

void zRPC_call_get_param(zRPC_call *call, const char *name, zRPC_value **value) {
    for (int i = 0; i < call->param_count; ++i) {
        if (strcmp(call->params[i].name, name) == 0) {
            *value = PASS_PTR(call->params[i].value, zRPC_value);
            break;
        }
    }
}

void zRPC_call_get_params(zRPC_call *call, zRPC_call_param **params, unsigned int *count) {
    *params = call->params;
    *count = call->param_count;
}

/* This function for call result */

void zRPC_call_result_create(zRPC_call_result **out) {
    zRPC_call_result *result = malloc(sizeof(zRPC_call_result));
    RTTI_INIT_PTR(zRPC_call_result, result);
    result->result_cab = 2;
    result->results = malloc(sizeof(*result->results) * result->result_cab);
    result->result_count = 0;
    *out = result;
}

void zRPC_call_result_destroy(zRPC_call_result *result) {
    if (result != NULL) {
        for (int i = 0; i < result->result_count; ++i) {
            free((void *) result->results[i].name);
            SUB_REFERENCE(result->results[i].value, zRPC_value);
        }
        free(result->results);
        free(result);
    }
}


void zRPC_call_result_set_result(zRPC_call_result *result, const char *name, zRPC_value *value) {
    if (result->result_count >= result->result_cab - 1) {
        result->result_cab *= 1.5;
        result->results = realloc(result->results, sizeof(*result->results) * result->result_cab);
    }
    result->results[result->result_count].name = malloc(strlen(name) + 1);
    bzero((void *) result->results[result->result_count].name, strlen(name) + 1);
    strcpy((char *) result->results[result->result_count].name, name);
    result->results[result->result_count++].value = value;
    SUB_REFERENCE(value, zRPC_value);
}


void zRPC_call_result_get_param(zRPC_call_result *result, const char *name, zRPC_value **value) {
    for (int i = 0; i < result->result_count; ++i) {
        if (strcmp(result->results[i].name, name) == 0) {
            *value = PASS_PTR(result->results[i].value, zRPC_value);
            break;
        }
    }
}


void zRPC_call_result_get_results(zRPC_call_result *result, zRPC_call_param **results, unsigned int *count) {
    *results = result->results;
    *count = result->result_count;
}