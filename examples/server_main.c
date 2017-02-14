#include "zRPC/context.h"
#include "zRPC/server.h"
#include "zRPC/rpc/callee.h"

int add(int a, int b) {
    return a + b;
}

void warp_callee_add(void *param, zRPC_caller_instance *caller_instance, zRPC_call *call, zRPC_call_result *result) {
    zRPC_value *a;
    zRPC_value *b;
    zRPC_call_get_param(call, "a", &a);
    zRPC_call_get_param(call, "b", &b);
    int64_t add_result = add((int) a->base_value->value.int64_value,
    (int) b->base_value->value.int64_value);
    zRPC_value *result_value
            = zRPC_type_var_create_base(PASS_PTR(zRPC_type_base_create(INT64, &add_result), zRPC_base_value));
    zRPC_call_result_set_result(result, "function_ret", PASS_PTR(result_value, zRPC_value));
    SUB_REFERENCE(a, zRPC_value);
    SUB_REFERENCE(b, zRPC_value);
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

    /*Init callee*/
    zRPC_callee *callee;
    zRPC_callee_create(&callee, NULL);

    zRPC_function_table_item function_table_callee[] = {
            {"add", NULL, warp_callee_add}
    };

    zRPC_callee_set_function_table(callee, function_table_callee,
                                   sizeof(function_table_callee) / sizeof(*function_table_callee));
    /*Init pipe*/

    zRPC_pipe *server_pipe;
    zRPC_pipe_create(&server_pipe);
    zRPC_filter_factory **filters;
    int filter_count;
    zRPC_callee_get_filters(callee, &filters, &filter_count);
    for (int i = 0; i < filter_count; ++i) {
        zRPC_pipe_add_filter(server_pipe, filters[i]);
    }

    /* Server start */
    zRPC_server *server = zRPC_server_create(context, "0.0.0.0:50000", server_pipe);
    zRPC_server_start(server);

    /*Dispatch context*/
    zRPC_context_dispatch(context);
}