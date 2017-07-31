//
// Created by zhsyourai on 2/5/17.
//

#ifndef ZRPC_LF_QUEUE_H
#define ZRPC_LF_QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
#include "../lf_base.h"
#include "../lf_backoff.h"

struct zRPC_queue_element {
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD next;
    void *value;
};

struct zRPC_queue_state {
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE enqueue;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE dequeue;
    struct zRPC_lfds_backoff_state dequeue_backoff, enqueue_backoff;
};

enum zRPC_queue_queue_state {
    ZRPC_QUEUE_QUEUE_STATE_UNKNOWN,
    ZRPC_QUEUE_QUEUE_STATE_EMPTY,
    ZRPC_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE,
    ZRPC_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE
};

void zRPC_lf_queue_init(struct zRPC_queue_state *q);

void zRPC_lf_queue_enqueue(struct zRPC_queue_state *q,
                           struct zRPC_queue_element *e);

int zRPC_lf_queue_dequeue(struct zRPC_queue_state *q,
                          struct zRPC_queue_element **e);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_LF_QUEUE_H
