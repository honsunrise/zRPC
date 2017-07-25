#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include <zRPC/rpc/call.h>
#include <zRPC/support/thread.h>
#include <zRPC/scheduling.h>
#include <zRPC/client.h>
#include <zRPC/rpc/call_stub.h>

void caller_add(zRPC_channel *channel, int64_t a, int64_t b, int64_t *ret) {
    zRPC_call_param *params = malloc(sizeof(zRPC_call_param) * 3);
    params[0].name = "a";
    params[0].value = zRPC_type_var_create_base(INT64, &a);
    params[1].name = "b";
    params[1].value = zRPC_type_var_create_base(INT64, &b);
    params[2].name = "c";
    params[2].value = zRPC_type_var_create_array(1000);
    for (int i = 0; i < 1000; ++i) {
        char v = (char) (i % 256);
        params[2].value->array_value->value[i] = zRPC_type_var_create_base(BYTE, &v);
    }
    zRPC_call *call = zRPC_call_do_call(channel, "add", params, 3);
    zRPC_call_result *result;
    zRPC_call_wait_result(call, &result);
    zRPC_call_destroy(call);
    zRPC_value *value;
    zRPC_call_result_get_param(result, "function_ret", &value);
    zRPC_call_result_destroy(result);
    free(params);
    *ret = value->int64_value;
    SUB_REFERENCE(value, zRPC_value);
}

typedef struct thread_param {
    zRPC_scheduler *context;
}thread_param;

int test_client_thread(void *arg) {
    thread_param *param = arg;

    zRPC_function_table_item function_table_callee[] = {
    };

    /*Init caller*/
    zRPC_call_stub *caller;
    zRPC_call_stub_create(&caller,
                          function_table_callee, sizeof(function_table_callee) / sizeof(*function_table_callee));

    /*Init pipe*/

    zRPC_pipe *client_pipe;
    zRPC_pipe_create(&client_pipe);
    zRPC_filter_factory **filters_client;
    int filter_count_client;
    zRPC_call_stub_get_filters(caller, &filters_client, &filter_count_client);
    for (int i = 0; i < filter_count_client; ++i) {
        zRPC_pipe_add_filter(client_pipe, filters_client[i]);
    }

    zRPC_client *client = zRPC_client_create(param->context, "127.0.0.1:50000", client_pipe);

    /*Client start*/
    zRPC_client_connect(client);

    sleep(10);

    volatile int i = 0;
    for (;;) {
        int a = rand() % 100;
        int b = rand() % 100;
        int64_t result;
        caller_add(zRPC_client_get_channel(client), a, b, &result);
        printf("%d add %d + %d result: %ld\n", i++, a, b, result);
    }
    return 0;
}


int main(int argc, char **argv) {
    srand((unsigned int) time(NULL));
    /*Init scheduler*/
    zRPC_scheduler *context = zRPC_context_create();

    thread_param *param = malloc(sizeof(thread_param));
    param->context = context;

    zRPC_thread_id thread_id;
    zRPC_thread_create(&thread_id, test_client_thread, param, PTHREAD_CREATE_DETACHED);

    /*Dispatch scheduler*/
    zRPC_context_dispatch(context);

    free(param);
    return 0;
}