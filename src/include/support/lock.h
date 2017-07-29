//
// Created by zhswo on 2016/11/22.
//

#ifndef ZRPC_LOCK_H
#define ZRPC_LOCK_H

#include <pthread.h>
#include <semaphore.h>

typedef pthread_mutex_t zRPC_mutex;
typedef pthread_cond_t zRPC_cond;
typedef pthread_once_t zRPC_once;
typedef sem_t zRPC_sem;

/*
 * Mutex interface
 */

int zRPC_mutex_init(zRPC_mutex *mutex);

int zRPC_mutex_destroy(zRPC_mutex *mutex);

int zRPC_mutex_lock(zRPC_mutex *mutex);

int zRPC_mutex_unlock(zRPC_mutex *mutex);

int zRPC_mutex_trylock(zRPC_mutex *mutex);

/*
 * Semaphore interface
 */

int zRPC_sem_init(zRPC_sem *sem, unsigned int value);

int zRPC_sem_wait(zRPC_sem *sem);

int zRPC_sem_post(zRPC_sem *sem);

int zRPC_sem_getvalue(zRPC_sem *sem, int *valp);

int zRPC_sem_destroy(zRPC_sem *sem);

/*
 * Condition variable interface
 */

int zRPC_cond_init(zRPC_cond *cond);

int zRPC_cond_destroy(zRPC_cond *cond);

int zRPC_cond_wait(zRPC_cond *cond, zRPC_mutex *mutex);

int zRPC_cond_wait_for(zRPC_cond *cond, zRPC_mutex *mutex);

int zRPC_cond_wait_until(zRPC_cond *cond, zRPC_mutex *mutex);

int zRPC_cond_notify_one(zRPC_cond *cond);

int zRPC_cond_notify_all(zRPC_cond *cond);

#define ZRPC_ONCE_INIT PTHREAD_ONCE_INIT

#endif //ZRPC_LOCK_H
