//
// Created by zhsyourai on 2/9/17.
//

#include "gtest/gtest.h"
#include "../src/lfds/lfds.h"
#include <boost/thread.hpp>

struct zRPC_queue_state queue;
int long run_insert_test() {
    struct zRPC_queue_element *e_p = (zRPC_queue_element *) malloc(sizeof(struct zRPC_queue_element));
    e_p->value = (void *) (random() + 1);
    zRPC_lf_queue_enqueue(&queue, e_p);
    return (int long)e_p->value;
}

int long run_get_test() {
    struct zRPC_queue_element *e;
    if(zRPC_lf_queue_dequeue(&queue, &e)) {
        int long value = (int long)e->value;
        free(e);
        return value;
    }
    return 0;
}

static void test_i() {
    volatile int i = 5000000;
    while (i--) {
        run_insert_test();
    }
}

static void test_r() {
    volatile int i = 5000000;
    while (1) {
        if(run_get_test()) {
            --i;
        }
        if(i <= 0) break;
    }
}

int long run_multi_thread_test() {
    boost::thread insert_thread1(test_i);
    boost::thread insert_thread2(test_i);
    boost::thread read_thread1(test_r);
    boost::thread read_thread2(test_r);
    insert_thread1.join();
    insert_thread2.join();
    read_thread1.join();
    read_thread2.join();
    return run_get_test();
}

TEST (QueueTests, InsertReadTest) {
    EXPECT_EQ (run_get_test(), run_insert_test());
}

TEST (QueueTests, MultiThreadTests) {
    EXPECT_EQ (0, run_multi_thread_test());
}

int main(int argc, char **argv) {
    srand(time(NULL));
  zRPC_lf_queue_init(&queue);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}