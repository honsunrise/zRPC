//
// Created by zhsyourai on 2/5/17.
//

#include <assert.h>
#include <malloc.h>
#include "lf_queue.h"

void zRPC_queue_init(struct zRPC_queue_state *q) {
    assert(q != NULL);
    assert((uintptr_t) &q->enqueue % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);
    assert((uintptr_t) &q->dequeue % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);
    struct zRPC_queue_element *e_dummy = malloc(sizeof(struct zRPC_queue_element));
    assert((uintptr_t) &e_dummy->next % ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD_LENGTH == 0);

    q->enqueue.ptr = e_dummy;
    q->enqueue.tag = 0;
    q->dequeue.ptr = e_dummy;
    q->dequeue.tag = 0;

    e_dummy->next.ptr = NULL;
    e_dummy->next.tag = 0;
    e_dummy->value = NULL;

    zRPC_lfds_misc_internal_backoff_init(&q->dequeue_backoff);
    zRPC_lfds_misc_internal_backoff_init(&q->enqueue_backoff);
    return;
}

void zRPC_queue_enqueue(struct zRPC_queue_state *q,
                        struct zRPC_queue_element *e) {
    assert(q != NULL);
    assert(e != NULL);

    unsigned char result = 0;
    unsigned char finished_flag = ZRPC_LFDS_FLAG_LOWERED;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;

    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD enqueue;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD new_enqueue;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD next;

    e->next.ptr = NULL;
    e->next.tag = 0;
    new_enqueue.ptr = e;
    do {
        /* TRD : note here the deviation from the white paper
                 in the white paper, next is loaded from enqueue, not from q->enqueue
                 what concerns me is that between the load of enqueue and the load of
                 enqueue->next, the element can be dequeued by another thread *and freed*
    
                 by ordering the loads (load barriers), and loading both from q,
                 the following if(), which checks enqueue is still the same as q->enqueue
                 still continues to ensure next belongs to enqueue, while avoiding the
                 problem with free
        */

        enqueue = q->enqueue;
        next = ((struct zRPC_queue_element *) q->enqueue.ptr)->next;
        if (TAGGED_POINT_COMPARE(q->enqueue, enqueue)) {
            if (next.ptr == NULL) {
                new_enqueue.tag = next.tag + 1;
                result = ZRPC_ATOM_ACQUIRE_DWCAS(&((struct zRPC_queue_element *) enqueue.ptr)->next, &next,
                                                 new_enqueue);
                if (result == 1)
                    finished_flag = ZRPC_LFDS_FLAG_RAISED;
            } else {
                next.tag = enqueue.tag + 1;
                // TRD : strictly, this is a weak CAS, but we do an extra iteration of the main loop on a fake failure, so we set it to be strong
                result = ZRPC_ATOM_ACQUIRE_DWCAS(&q->enqueue, &enqueue, next);
            }
        } else
            result = 0;

        if (result == 0)
            zRPC_lfds_backoff_exponential_backoff(&q->enqueue_backoff, &backoff_iteration);
    } while (finished_flag == ZRPC_LFDS_FLAG_LOWERED);

    // TRD : move enqueue along; only a weak CAS as the dequeue will solve this if it's out of place
    new_enqueue.tag = enqueue.tag + 1;
    result = ZRPC_ATOM_ACQUIRE_DWCAS(&q->enqueue, &enqueue, new_enqueue);

    if (result == 0)
        zRPC_lfds_backoff_exponential_backoff(&q->enqueue_backoff, &backoff_iteration);

    zRPC_lfds_backoff_autotune(&q->enqueue_backoff, backoff_iteration);

    return;
}


int zRPC_queue_dequeue(struct zRPC_queue_state *q,
                       struct zRPC_queue_element **e) {
    assert(q != NULL);
    assert(e != NULL);

    unsigned char result = 0;
    unsigned char finished_flag = ZRPC_LFDS_FLAG_LOWERED;
    enum zRPC_queue_queue_state state = ZRPC_QUEUE_QUEUE_STATE_UNKNOWN;
    int rv = 1;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;

    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD dequeue;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD enqueue;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD next;

    void *key = NULL, *value = NULL;
    do {
        /* TRD : note here the deviation from the white paper
                 in the white paper, next is loaded from dequeue, not from q->dequeue
                 what concerns me is that between the load of dequeue and the load of
                 enqueue->next, the element can be dequeued by another thread *and freed*
    
                 by ordering the loads (load barriers), and loading both from q,
                 the following if(), which checks dequeue is still the same as q->enqueue
                 still continues to ensure next belongs to enqueue, while avoiding the
                 problem with free
        */

        dequeue = q->dequeue;
        enqueue = q->enqueue;
        next = ((struct zRPC_queue_element *) q->dequeue.ptr)->next;

        if (TAGGED_POINT_COMPARE(q->dequeue, dequeue)) {
            if (enqueue.ptr == dequeue.ptr &&
                next.ptr == NULL)
                state = ZRPC_QUEUE_QUEUE_STATE_EMPTY;

            if (enqueue.ptr == dequeue.ptr &&
                next.ptr != NULL)
                state = ZRPC_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE;

            if (enqueue.ptr != dequeue.ptr)
                state = ZRPC_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE;

            switch (state) {
                case ZRPC_QUEUE_QUEUE_STATE_UNKNOWN:
                    // TRD : eliminates compiler warning
                    break;

                case ZRPC_QUEUE_QUEUE_STATE_EMPTY:
                    rv = 0;
                    *e = NULL;
                    result = 1;
                    finished_flag = ZRPC_LFDS_FLAG_RAISED;
                    break;

                case ZRPC_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE:
                    next.tag = enqueue.tag + 1;
                    result = ZRPC_ATOM_ACQUIRE_DWCAS(&q->enqueue, &enqueue, next);
                    // TRD : in fact if result is 1 (successful) I think we can now simply drop down into the dequeue attempt
                    break;

                case ZRPC_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE:
                    value = ((struct zRPC_queue_element *) next.ptr)->value;
                    next.tag = dequeue.tag + 1;
                    result = ZRPC_ATOM_ACQUIRE_DWCAS(&q->dequeue, &dequeue, next);
                    if (result == 1)
                        finished_flag = ZRPC_LFDS_FLAG_RAISED;
                    break;
            }
        } else
            result = 0;

        if (result == 0)
            zRPC_lfds_backoff_exponential_backoff(&q->dequeue_backoff, &backoff_iteration);
    } while (finished_flag == ZRPC_LFDS_FLAG_LOWERED);

    if (result == 1) {
        *e = dequeue.ptr;
        (*e)->value = value;
    }

    zRPC_lfds_backoff_autotune(&q->dequeue_backoff, backoff_iteration);

    return rv;
}