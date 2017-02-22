#include <zRPC/rpc/call.h>
#include <zRPC/rpc/call_stub.h>
#include <zRPC/server.h>

int add(int a, int b) {
    return a + b;
}

void warp_callee_add(void *param, zRPC_caller_instance *caller_instance, zRPC_call *call, zRPC_call_result *result) {
    static volatile int i = 0;
    zRPC_value *a;
    zRPC_value *b;
    zRPC_value *c;
    zRPC_call_get_param(call, "a", &a);
    zRPC_call_get_param(call, "b", &b);
    zRPC_call_get_param(call, "c", &c);
    int64_t add_result = add((int) a->int64_value, (int) b->int64_value);
    zRPC_value *result_value = zRPC_type_var_create_base(INT64, &add_result);
    result->request_id = call->request_id;
    zRPC_call_result_set_result(result, "function_ret", PASS_PTR(result_value, zRPC_value));
    SUB_REFERENCE(a, zRPC_value);
    SUB_REFERENCE(b, zRPC_value);
    SUB_REFERENCE(c, zRPC_value);
    printf("%d add\n", i++);
}

typedef struct test_node {
    int a;
    zRPC_list_head list_node;
} test_node;

test_node *getNode(int a) {
    test_node *node = malloc(sizeof(test_node));
    node->a = a;
    return node;
}

int main(int argc, char **argv) {
    /*Init context*/
    zRPC_context *context = zRPC_context_create();

    zRPC_function_table_item function_table_callee[] = {
            {"add", NULL, warp_callee_add}
    };

    /*Init callee*/
    zRPC_call_stub *callee;
    zRPC_call_stub_create(&callee,
                          function_table_callee, sizeof(function_table_callee) / sizeof(*function_table_callee));

    /*Init pipe*/
    zRPC_pipe *server_pipe;
    zRPC_pipe_create(&server_pipe);
    zRPC_filter_factory **filters;
    int filter_count;
    zRPC_call_stub_get_filters(callee, &filters, &filter_count);
    for (int i = 0; i < filter_count; ++i) {
        zRPC_pipe_add_filter(server_pipe, filters[i]);
    }

    /* Server start */
    zRPC_server *server = zRPC_server_create(context, "0.0.0.0:50000", server_pipe);
    zRPC_server_start(server);

    /*Dispatch context*/
    zRPC_context_dispatch(context);
}