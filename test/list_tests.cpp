//
// Created by zhsyourai on 2/9/17.
//

#include "gtest/gtest.h"
#include "../src/lfds/lfds.h"
#include <boost/thread.hpp>

struct zRPC_lfds_list_state list;
int long run_insert_test() {
    struct zRPC_lfds_list_element *e_p = (zRPC_lfds_list_element *) malloc(sizeof(struct zRPC_lfds_list_element));
    e_p->value = (void *) 'H';
    zRPC_lfds_list_insert_at_start(&list, e_p);
    return (int long)e_p->value;
}

int long get_list_count() {
    return zRPC_lfds_list_count(&list);
}

static void test_i() {
    volatile int i = 5000000;
    while (i--) {
        run_insert_test();
    }
}

int long run_multi_thread_test() {
    boost::thread insert_thread1(test_i);
    boost::thread insert_thread2(test_i);
    insert_thread1.join();
    insert_thread2.join();
    return get_list_count();
}

TEST (ListTests, InsertReadTest) {
    EXPECT_EQ ('H', run_insert_test());
}

TEST (ListTests, MultiThreadTests) {
    EXPECT_EQ (5000000 * 2 + 1, run_multi_thread_test());
}

int main(int argc, char **argv) {
    srand(time(NULL));
    zRPC_lfds_list_init(&list);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}