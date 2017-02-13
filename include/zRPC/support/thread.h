//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_THREAD_H
#define ZRPC_THREAD_H
#ifdef __cplusplus
extern "C" {
#endif

#define ZRPC_THREAD_FLAG_JOINABLE 1

#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>

typedef uintptr_t zRPC_thread_id;

/* create a new thread */
int zRPC_thread_create(zRPC_thread_id *thread_id, int (*worker)(void *), void *arg, int flag);

zRPC_thread_id zRPC_thread_current_id(void);

void zRPC_thread_join(zRPC_thread_id thread_id);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_THREAD_H
