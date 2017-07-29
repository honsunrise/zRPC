//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_RUNNABLE_H
#define ZRPC_RUNNABLE_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct zRPC_runnable zRPC_runnable;

typedef struct zRPC_runnable_holder zRPC_runnable_holder;

zRPC_runnable *zRPC_runnable_create(void *(*run)(void *), void *arg, void (*run_callback)(zRPC_runnable_holder *));

void zRPC_runnable_destroy(zRPC_runnable *runnable);

void zRPC_runnable_run(zRPC_runnable *runnable);

void zRPC_runnable_release_callback(zRPC_runnable_holder *);

#define zRPC_runnable_noting_callback (NULL)
#ifdef __cplusplus
}
#endif
#endif //ZRPC_RUNABLE_H
