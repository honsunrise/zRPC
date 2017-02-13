//
// Created by zhsyourai on 1/22/17.
//

#include <thread>
#include "zRPC/context.h"
#include "gtest/gtest.h"

static volatile int run_times = 0;
void run(zRPC_context *context) {
    ++run_times;
    zRPC_runnable *runnable = zRPC_runnable_create((void *(*)(void *)) run, context, zRPC_runnable_release_callback);
    zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
    deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(1, zRPC_CLOCK_MONOTONIC));
    zRPC_timer_schedule(context, deadline, runnable);
}

TEST (TimerTest, PositiveNos) {
    EXPECT_LT (0, run_times);
}

void test() {
    zRPC_context *context = zRPC_context_create();
    zRPC_runnable *runnable = zRPC_runnable_create((void *(*)(void *)) run, context, zRPC_runnable_release_callback);
    zRPC_timespec deadline = zRPC_now(zRPC_CLOCK_MONOTONIC);
    deadline = zRPC_time_add(deadline, zRPC_time_from_seconds(1, zRPC_CLOCK_MONOTONIC));
    zRPC_timer_schedule(context, deadline, runnable);
    zRPC_context_dispatch(context);
}

int main(int argc, char **argv) {
    std::thread test_thread(test);
    test_thread.detach();
    sleep(5);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}