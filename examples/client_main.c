#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include "zRPC/support/thread.h"
#include "zRPC/context.h"
#include "zRPC/rpc/callee.h"
#include "zRPC/client.h"
#include "zRPC/rpc/caller.h"

int caller_add(zRPC_caller *caller, zRPC_client *client, int64_t a, int64_t b, int64_t *ret) {
    zRPC_call_param *params = malloc(sizeof(zRPC_call_param) * 2);
    params[0].name = "a";
    params[0].value = zRPC_type_var_create_base(zRPC_type_base_create(INT64, &a));
    params[1].name = "b";
    params[1].value = zRPC_type_var_create_base(zRPC_type_base_create(INT64, &b));
    zRPC_call *call = zRPC_caller_do_call(caller, client, "add", params, 2);
    zRPC_call_result *result;
    zRPC_caller_wait_result(caller, call, &result);
    zRPC_call_destroy(call);
    zRPC_value *value;
    zRPC_call_result_get_param(result, "function_ret", &value);
    zRPC_caller_destroy_result(caller, result);
    free(params);
    *ret = value->base_value->value.int64_value;
    return *ret;
}

typedef struct thread_param {
    zRPC_caller *caller;
    zRPC_client *client;
}thread_param;

int test_client_thread(void *arg) {
    sleep(1);
    thread_param *param = arg;
    volatile int i = 0;
    for (;;) {
        int a = rand() % 100;
        int b = rand() % 100;
        int64_t result;
        int ret = caller_add(param->caller, param->client, a, b, &result);
        if (ret == 0)
            break;
        printf("%d add %d + %d result: %ld\n", i++, a, b, result);
    }
    return 0;
}


int main(int argc, char **argv) {
    srand((unsigned int) time(NULL));
    /*Init context*/
    zRPC_context *context = zRPC_context_create();

    /*Init caller*/
    zRPC_caller *caller;
    zRPC_caller_create(&caller);

    /*Init pipe*/

    zRPC_pipe *client_pipe;
    zRPC_pipe_create(&client_pipe);
    zRPC_filter_factory **filters_client;
    int filter_count_client;
    zRPC_caller_get_filters(caller, &filters_client, &filter_count_client);
    for (int i = 0; i < filter_count_client; ++i) {
        zRPC_pipe_add_filter(client_pipe, filters_client[i]);
    }

    zRPC_client *client = zRPC_client_create(context, "127.0.0.1:50000", client_pipe);

    thread_param *param = malloc(sizeof(thread_param));
    param->caller = caller;
    param->client = client;

    zRPC_thread_id thread_id;
    zRPC_thread_create(&thread_id, test_client_thread, param, PTHREAD_CREATE_DETACHED);

    /*Client start*/
    zRPC_client_start(client);

    /*Dispatch context*/
    zRPC_context_dispatch(context);

    free(param);
    return 0;
}