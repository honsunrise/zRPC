//
// Created by zhswo on 2016/11/22.
//

#include "zRPC/support/lock.h"

int zRPC_mutex_init(zRPC_mutex *mutex) {
    return pthread_mutex_init(mutex, NULL);
}

int zRPC_mutex_destroy(zRPC_mutex *mutex) {
    return pthread_mutex_destroy(mutex);
}

int zRPC_mutex_lock(zRPC_mutex *mutex) {
    return pthread_mutex_lock(mutex);
}

int zRPC_mutex_unlock(zRPC_mutex *mutex) {
    return pthread_mutex_unlock(mutex);
}

int zRPC_mutex_trylock(zRPC_mutex *mutex) {
    return pthread_mutex_trylock(mutex) == 0;
}

/*
 * Semaphore interface
 */

int zRPC_sem_init(zRPC_sem *sem, unsigned int value){
    return sem_init(sem, 0, value);
}

int zRPC_sem_wait(zRPC_sem *sem) {
    return sem_wait(sem);
}

int zRPC_sem_post(zRPC_sem *sem) {
    return sem_post(sem);
}

int zRPC_sem_getvalue(zRPC_sem *sem, int *valp) {
    return sem_getvalue(sem, valp);
}

int zRPC_sem_destroy(zRPC_sem *sem){
    return sem_destroy(sem);
}
/*
 * Condition variable interface
 */

int zRPC_cond_init(zRPC_cond *cond) {
    return pthread_cond_init(cond, NULL);
}

int zRPC_cond_destroy(zRPC_cond *cond) {
    return pthread_cond_destroy(cond);
}

int zRPC_cond_wait(zRPC_cond *cond, zRPC_mutex *mutex) {
    return pthread_cond_wait(cond, mutex);
}

int zRPC_cond_wait_for(zRPC_cond *cond, zRPC_mutex *mutex) {}

int zRPC_cond_wait_until(zRPC_cond *cond, zRPC_mutex *mutex) {}

int zRPC_cond_notify_one(zRPC_cond *cond) {
    return pthread_cond_signal(cond);
}

int zRPC_cond_notify_all(zRPC_cond *cond) {
    return pthread_cond_broadcast(cond);
}