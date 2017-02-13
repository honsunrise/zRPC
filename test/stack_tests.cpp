//
// Created by zhsyourai on 2/9/17.
//

#include "gtest/gtest.h"
#include "../src/lfds/lfds.h"
#include <thread>

struct zRPC_stack_state list;
int long run_insert_test() {
    struct zRPC_stack_element *e_p = (zRPC_stack_element *) malloc(sizeof(struct zRPC_stack_element));
    e_p->value = (void *) (random() + 1);
    zRPC_stack_push(&list, e_p);
    return (int long)e_p->value;
}

int long run_get_test() {
    struct zRPC_stack_element *e;
    if(zRPC_stack_pop(&list, &e)) {
        int long value = (int long)e->value;
        free(e);
        return value;
    }
    return 0;
}

int long run_multi_thread_test() {
    std::thread insert_thread1([](){
        volatile int i = 5000000;
        while (i--) {
            run_insert_test();
        }
    });
    std::thread insert_thread2([](){
        volatile int i = 5000000;
        while (i--) {
            run_insert_test();
        }
    });
    std::thread read_thread1([](){
        volatile int i = 5000000;
        while (1) {
            if(run_get_test()) {
                --i;
            }
            if(i <= 0) break;
        }
    });
    std::thread read_thread2([](){
        volatile int i = 5000000;
        while (1) {
            if(run_get_test()) {
                --i;
            }
            if(i <= 0) break;
        }
    });
    insert_thread1.join();
    insert_thread2.join();
    read_thread1.join();
    read_thread2.join();
    return run_get_test();
}

TEST (StackTests, InsertReadTest) {
    EXPECT_EQ (run_get_test(), run_insert_test());
}

TEST (StackTests, MultiThreadTests) {
    EXPECT_EQ (0, run_multi_thread_test());
}

int main(int argc, char **argv) {
    srand(time(NULL));
    zRPC_stack_init(&list);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}