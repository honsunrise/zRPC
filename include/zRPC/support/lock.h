//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_LOCK_H
#define ZRPC_LOCK_H

#include <pthread.h>

typedef pthread_mutex_t zRPC_mutex;
typedef pthread_cond_t zRPC_cond;
typedef pthread_once_t zRPC_once;

/*
 * Mutex interface
 */

void zRPC_mutex_init(zRPC_mutex *mutex);

void zRPC_mutex_destroy(zRPC_mutex *mutex);

void zRPC_mutex_lock(zRPC_mutex *mutex);

void zRPC_mutex_unlock(zRPC_mutex *mutex);

int zRPC_mutex_trylock(zRPC_mutex *mutex);

/*
 * Condition variable interface
 */

void zRPC_cond_init(zRPC_cond *cond);

void zRPC_cond_destroy(zRPC_cond *cond);

void zRPC_cond_wait(zRPC_cond *cond, zRPC_mutex *mutex);

void zRPC_cond_wait_for(zRPC_cond *cond, zRPC_mutex *mutex);

void zRPC_cond_wait_until(zRPC_cond *cond, zRPC_mutex *mutex);

void zRPC_cond_notify_one(zRPC_cond *cond);

void zRPC_cond_notify_all(zRPC_cond *cond);

#define ZRPC_ONCE_INIT PTHREAD_ONCE_INIT

#endif //ZRPC_LOCK_H
