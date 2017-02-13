//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_WORK_QUEUE_H
#define ZRPC_WORK_QUEUE_H

#include "runnable.h"

typedef struct zRPC_queue zRPC_queue;

int zRPC_queue_create(zRPC_queue **queue);

void zRPC_queue_destroy(zRPC_queue *queue);

int zRPC_queue_is_empty(zRPC_queue *queue);

int zRPC_queue_is_full(zRPC_queue *queue);

int zRPC_queue_enqueue(zRPC_queue *queue, void *entry);

int zRPC_queue_dequeue(zRPC_queue *queue, void **entry);

#endif //ZRPC_WORK_QUEUE_H
