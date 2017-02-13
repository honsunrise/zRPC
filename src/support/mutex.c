//
// Created by zhswo on 2016/11/22.
//

#include "zRPC/support/lock.h"

void zRPC_mutex_init(zRPC_mutex *mutex) {
    pthread_mutex_init(mutex, NULL);
}

void zRPC_mutex_destroy(zRPC_mutex *mutex) {
    pthread_mutex_destroy(mutex);
}

void zRPC_mutex_lock(zRPC_mutex *mutex) {
    pthread_mutex_lock(mutex);
}

void zRPC_mutex_unlock(zRPC_mutex *mutex) {
    pthread_mutex_unlock(mutex);
}

int zRPC_mutex_trylock(zRPC_mutex *mutex) {
    return pthread_mutex_trylock(mutex) == 0;
}

/*
 * Condition variable interface
 */

void zRPC_cond_init(zRPC_cond *cond) {
    pthread_cond_init(cond, NULL);
}

void zRPC_cond_destroy(zRPC_cond *cond) {
    pthread_cond_destroy(cond);
}

void zRPC_cond_wait(zRPC_cond *cond, zRPC_mutex *mutex) {
    pthread_cond_wait(cond, mutex);
}

void zRPC_cond_wait_for(zRPC_cond *cond, zRPC_mutex *mutex) {}

void zRPC_cond_wait_until(zRPC_cond *cond, zRPC_mutex *mutex) {}

void zRPC_cond_notify_one(zRPC_cond *cond) {
    pthread_cond_signal(cond);
}

void zRPC_cond_notify_all(zRPC_cond *cond) {
    pthread_cond_broadcast(cond);
}