//
// Created by zhswo on 2016/11/23.
//

#include <malloc.h>
#include "support/useful.h"
#include "support/runnable.h"

struct zRPC_runnable {
    void *arg;

    void *(*run)(void *);
};

struct zRPC_runnable_holder {
    void (*run_callback)(struct zRPC_runnable_holder *run);

    zRPC_runnable runnable;
};

zRPC_runnable *zRPC_runnable_create(void *(*run)(void *), void *arg, void (*run_callback)(zRPC_runnable_holder *)) {
    zRPC_runnable_holder *runnable_holder = malloc(sizeof(zRPC_runnable_holder));
    runnable_holder->runnable.run = run;
    runnable_holder->runnable.arg = arg;
    runnable_holder->run_callback = run_callback;
    return &runnable_holder->runnable;
}

void zRPC_runnable_destroy(zRPC_runnable *runnable) {
    zRPC_runnable_holder *runnable_holder = container_of(runnable, zRPC_runnable_holder, runnable);
    free(runnable_holder);
}

static void zRPC_runnable_run_callback(zRPC_runnable *runnable) {
    zRPC_runnable_holder *runnable_holder = container_of(runnable, zRPC_runnable_holder, runnable);
    if (runnable_holder->run_callback)
        runnable_holder->run_callback(runnable_holder);
}

void zRPC_runnable_run(zRPC_runnable *runnable) {
    runnable->run(runnable->arg);
    zRPC_runnable_run_callback(runnable);
}

void zRPC_runnable_release_callback(zRPC_runnable_holder *runnable_holder) {
    free(runnable_holder);
}