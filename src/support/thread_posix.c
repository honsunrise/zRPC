//
// Created by zhswo on 2016/11/22.
//

#include <malloc.h>
#include "zRPC/support/thread.h"

typedef struct zRPC_THREAD {
    int thread_id;
    int flag;
    void *arg;

    int (*worker)(void *arg);
} zRPC_THREAD;


/* mock body for every thread */
static void *thread_body(void *argv) {
    /* copy to local variable*/
    zRPC_THREAD t = *(zRPC_THREAD *) argv;
    free(argv);
    (*t.worker)(t.arg);
    return 0;
}

int zRPC_thread_create(zRPC_thread_id *thread_id, int (*worker)(void *), void *arg, int flag) {
    int pthread_ret;
    pthread_attr_t attr;
    pthread_t p;

    zRPC_THREAD *thread = malloc(sizeof(zRPC_THREAD));
    thread->worker = worker;
    thread->flag = flag;
    thread->arg = arg;

    pthread_attr_init(&attr);
    if (flag & ZRPC_THREAD_FLAG_JOINABLE) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    } else {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }
    pthread_ret = (pthread_create(&p, &attr, &thread_body, thread) == 0);
    pthread_attr_destroy(&attr);
    if (!pthread_ret) {
        free(thread);
    }
    *thread_id = (zRPC_thread_id) p;
    return pthread_ret;
}

zRPC_thread_id zRPC_thread_current_id(void) {
    return (zRPC_thread_id) pthread_self();
}

void zRPC_thread_join(zRPC_thread_id thread_id) {
    pthread_join((pthread_t) thread_id, NULL);
}