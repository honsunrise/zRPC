//
// Created by zhsyourai on 2/9/17.
//

#include "gtest/gtest.h"
#include <boost/thread.hpp>
#include <zRPC/filter/msgpack_filter.h>
#include <zRPC/rpc/call.h>

zRPC_filter *filter;
static void test_i() {
    int64_t param = 10;
    zRPC_call *call;
    zRPC_call_create(&call);
    call->request_id = 0;
    zRPC_call_set_function(call, "test");
    zRPC_call_set_param(call, "test_param", zRPC_type_var_create_base(INT64, &param));
    volatile int i = 500000;
    while (i--) {
        zRPC_filter_out *out_w;
        zRPC_filter_out *out_r;
        zRPC_filter_out_create(&out_w);
        zRPC_filter_out_create(&out_r);
        msgpack_filter_on_writable(filter, NULL, call, out_w);
        msgpack_filter_on_readable(filter, NULL, zRPC_filter_out_get_item(out_w, 0), out_r);
        zRPC_call_destroy((zRPC_call *) zRPC_filter_out_get_item(out_r, 0));
        zRPC_filter_out_destroy(out_w);
        zRPC_filter_out_destroy(out_r);
    }
}


int long run_test() {
    boost::thread insert_thread1(test_i);
    insert_thread1.join();
    return 0;
}

TEST (FilterTests, Test) {
    EXPECT_EQ (0, run_test());
}

int main(int argc, char **argv) {
    zRPC_filter_factory *factory = msgpack_filter_factory();
    zRPC_filter_create_by_factory(&filter, factory);
    srand(time(NULL));
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}