//
// Created by zhsyourai on 1/22/17.
//

#include "zRPC/context.h"
#include "gtest/gtest.h"
#include <boost/thread.hpp>

volatile size_t naddres = 0;
void resolver_complete_callback(void *custom_arg, zRPC_resolved *resolved) {
    naddres = resolved->inetaddres.naddrs;
}

void test() {
    const char *hostname = "www.google.com:80";
    zRPC_context *context = zRPC_context_create();
    zRPC_resolver_address(context, hostname, resolver_complete_callback, NULL);
    zRPC_context_dispatch(context);
}

TEST (ResolverTest, PositiveNos) {
    EXPECT_LT (0, naddres);
}

int main(int argc, char **argv) {
    boost::thread test_thread(test);
    test_thread.detach();
    sleep(5);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}