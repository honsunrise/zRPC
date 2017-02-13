//
// Created by zhsyourai on 2/10/17.
//

#include "lf_list.h"

void zRPC_list_init(struct zRPC_list_state *l) {
    assert(l != NULL);
    assert((uintptr_t) &l->dummy_element % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);
    assert((uintptr_t) &l->end % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);
    assert((uintptr_t) &l->start % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);

    l->dummy_element.next.ptr = NULL;
    l->dummy_element.next.tag = 0;
    l->dummy_element.value = NULL;
    l->start.ptr = l->end.ptr = &l->dummy_element;
    l->start.tag = l->end.tag = 0;

    zRPC_lfds_misc_internal_backoff_init(&l->after_backoff);
    zRPC_lfds_misc_internal_backoff_init(&l->start_backoff);
    zRPC_lfds_misc_internal_backoff_init(&l->end_backoff);
    return;
}

void zRPC_list_insert_at_start(struct zRPC_list_state *l,
                               struct zRPC_list_element *e) {
    assert(l != NULL);
    assert(e != NULL);
    assert((uintptr_t) &e->next % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);
    assert((uintptr_t) &e->value % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);
    unsigned char result;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE new_start;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE original_start;

    original_start =  l->start;
    new_start.ptr = e;

    do {
        e->next = original_start;
        new_start.tag = original_start.tag + 1;
        result = ZRPC_ATOM_ACQUIRE_DWCAS(&l->start, &original_start, new_start);
        if (result == 0) {
            zRPC_lfds_backoff_exponential_backoff(&l->start_backoff, &backoff_iteration);
        }
    } while (result == 0);

    zRPC_lfds_backoff_autotune(&l->start_backoff, backoff_iteration);

    return;
}

void zRPC_list_insert_at_end(struct zRPC_list_state *l,
                             struct zRPC_list_element *e) {
    unsigned char result;
    unsigned char finished_flag = ZRPC_LFDS_FLAG_LOWERED;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;

    assert(l != NULL);
    assert(e != NULL);
    assert((uintptr_t) &e->next % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);
    assert((uintptr_t) &e->value % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);

    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE new_end;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE original_end;

    e->next.ptr = NULL;
    e->next.tag = 0;
    new_end.ptr = e;
    original_end = l->end;

    while (finished_flag == ZRPC_LFDS_FLAG_LOWERED) {
        new_end.tag = original_end.tag + 1;
        result = ZRPC_ATOM_ACQUIRE_DWCAS(&l->end, &original_end, new_end);

        if (result == 1)
            finished_flag = ZRPC_LFDS_FLAG_RAISED;
        else {
            zRPC_lfds_backoff_exponential_backoff(&l->end_backoff, &backoff_iteration);

            struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE next = original_end;
            while (next.ptr != NULL) {
                next = l->end;
            }
            original_end = next;
        }
    }

    zRPC_lfds_backoff_autotune(&l->end_backoff, backoff_iteration);

    return;
}

void zRPC_list_insert_after_element(struct zRPC_list_state *l,
                                    struct zRPC_list_element *e,
                                    struct zRPC_list_element *e_pre) {
    unsigned char result;
    uintptr_t backoff_iteration = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;
    assert(l != NULL);
    assert(e != NULL);
    assert((uintptr_t) &e->next % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);
    assert((uintptr_t) &e->value % ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH == 0);
    assert(e_pre != NULL);

    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE new_next;

    new_next.ptr = e;
    e->next = e_pre->next;
    do {
        new_next.tag = e->next.tag + 1;
        result = ZRPC_ATOM_ACQUIRE_DWCAS(&e_pre->next, &e->next, new_next);
        if (result == 0)
            zRPC_lfds_backoff_exponential_backoff(&l->after_backoff, &backoff_iteration);
    } while (result == 0);

    zRPC_lfds_backoff_autotune(&l->after_backoff, backoff_iteration);

    return;
}

size_t zRPC_list_count(struct zRPC_list_state *l) {
    struct zRPC_list_element *le = (struct zRPC_list_element *) l->start.ptr;
    size_t count = 0;
    while (le->next.ptr != NULL) {
        ++count;
        le = (struct zRPC_list_element *)(le->next.ptr);
    }
    return count;
}
