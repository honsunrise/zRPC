//
// Created by zhswo on 2016/11/23.
//

#include <malloc.h>
#include <memory.h>
#include "zRPC/ds/queue.h"

/* this number for programmer day*/
#define MAX_QUEUE_CAP 1024

struct zRPC_queue {
    zRPC_runnable *entities[MAX_QUEUE_CAP];
    unsigned int cap;
    int front;
    int rear;
    unsigned int count;
};

int zRPC_queue_create(zRPC_queue **queue) {
    zRPC_queue *q = malloc(sizeof(zRPC_queue));
    if (q <= 0)
        return -1;
    bzero(q->entities, sizeof(q->entities));
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->cap = MAX_QUEUE_CAP;
    *queue = q;
    return 0;
}

void zRPC_queue_destroy(zRPC_queue *queue) {
    free(queue);
}

int zRPC_queue_is_empty(zRPC_queue *queue) {
    return queue->front == queue->rear;
}

int zRPC_queue_is_full(zRPC_queue *queue) {
    return queue->front == ((queue->rear + 1) % queue->cap);
}

int zRPC_queue_enqueue(zRPC_queue *queue, void *entry) {
    if (zRPC_queue_is_full(queue)) {
        return 0;
    } else {
        queue->entities[queue->rear] = entry;
        queue->rear = ((queue->rear + 1) % queue->cap);
        queue->count++;
        return 1;
    }
}

int zRPC_queue_dequeue(zRPC_queue *queue, void **entry) {
    if (zRPC_queue_is_empty(queue)) {
        *entry = NULL;
        return 0;
    } else {
        *entry = queue->entities[queue->front];
        queue->front = ((queue->front + 1) % queue->cap);
        queue->count--;
        return 1;
    }
}