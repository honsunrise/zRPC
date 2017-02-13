//
// Created by zhsyourai on 2/5/17.
//

#include "lf_stack.h"

void zRPC_stack_init(struct zRPC_stack_state *ss) {
    assert(ss != NULL);
    assert((uintptr_t) &ss->top % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);
    ss->top.ptr = NULL;
    ss->top.tag = 0;
    zRPC_lfds_misc_internal_backoff_init(&ss->pop_backoff);
    zRPC_lfds_misc_internal_backoff_init(&ss->push_backoff);
    return;
}

void zRPC_stack_cleanup(struct zRPC_stack_state *ss,
                        void (*element_cleanup_callback)(struct zRPC_stack_state *ss,
                                                         struct zRPC_stack_element *se)) {
    struct zRPC_stack_element *se, *se_temp;

    assert(ss != NULL);

    if (element_cleanup_callback != NULL) {
        se = ss->top.ptr;

        while (se != NULL) {
            se_temp = se;
            se = se->next;
            element_cleanup_callback(ss, se_temp);
        }
    }

    return;
}

int zRPC_stack_pop(struct zRPC_stack_state *ss,
                   struct zRPC_stack_element **se) {
    unsigned char result;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;

    tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE new_top;
    tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE original_top;

    assert(ss != NULL);
    assert(se != NULL);

    original_top = ss->top;
    do {
        if (original_top.ptr == NULL) {
            *se = NULL;
            return 0;
        }

        new_top.tag = original_top.tag + 1;
        new_top.ptr = ((struct zRPC_stack_element *) original_top.ptr)->next;
        result = ZRPC_ATOM_ACQUIRE_DWCAS(&ss->top, &original_top, new_top);
        if (result == 0) {
            zRPC_lfds_backoff_exponential_backoff(&ss->pop_backoff, &backoff_iteration);
        }
    } while (result == 0);

    *se = (struct zRPC_stack_element *) original_top.ptr;

    zRPC_lfds_backoff_autotune(&ss->pop_backoff, backoff_iteration);

    return 1;
}

void zRPC_stack_push(struct zRPC_stack_state *ss, struct zRPC_stack_element *se) {
    unsigned char result;

    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;

    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE new_top;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE original_top;

    assert(ss != NULL);
    assert(se != NULL);

    new_top.ptr = se;

    original_top = ss->top;

    do {
        se->next = original_top.ptr;
        new_top.tag = original_top.tag + 1;
        result = ZRPC_ATOM_ACQUIRE_DWCAS(&ss->top, &original_top, new_top);
        if (result == 0) {
            zRPC_lfds_backoff_exponential_backoff(&ss->push_backoff, &backoff_iteration);
        }
    } while (result == 0);

    zRPC_lfds_backoff_autotune(&ss->push_backoff, backoff_iteration);
    return;
}

size_t zRPC_stack_count(struct zRPC_stack_state *ss) {
    struct zRPC_stack_element *se = (struct zRPC_stack_element *) ss->top.ptr;
    size_t count = 0;
    while (se != NULL) {
        ++count;
        se = (struct zRPC_stack_element *) se->next;
    }
    return count;
}
