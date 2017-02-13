//
// Created by zhsyourai on 2/5/17.
//

#ifndef ZRPC_LF_BACKOFF_H
#define ZRPC_LF_BACKOFF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "lf_base.h"

struct zRPC_lfds_backoff_state {
    zRPC_atom ZRPC_ATOM_ARCH_ALIG_CACHELINE lock;
    zRPC_atom backoff_iteration_frequency_counters[2], metric, total_operations;
};

#define ZRPC_LFDS_BACKOFF_INITIAL_VALUE  0
#define ZRPC_LFDS_BACKOFF_LIMIT          10

static void inline zRPC_lfds_backoff_exponential_backoff(struct zRPC_lfds_backoff_state *backoff_state, uintptr_t *backoff_iteration) {
    uintptr_t volatile loop;
    uintptr_t endloop;

    if ((*backoff_iteration) == ZRPC_LFDS_BACKOFF_LIMIT) {
        (*backoff_iteration) = ZRPC_LFDS_BACKOFF_INITIAL_VALUE;
    } else {
        endloop = (((uintptr_t) 0x1) << *backoff_iteration) * backoff_state->metric;
        for (loop = 0; loop < endloop; loop++);
    }
    (*backoff_iteration)++;
}

static void inline zRPC_lfds_backoff_autotune(struct zRPC_lfds_backoff_state *bs, uintptr_t backoff_iteration) {
    if (backoff_iteration < 2)
        ++bs->backoff_iteration_frequency_counters[backoff_iteration];

    if (++bs->total_operations >= 10000
        && bs->lock == ZRPC_LFDS_FLAG_LOWERED) {
        zRPC_atom e = ZRPC_LFDS_FLAG_LOWERED;
        zRPC_atom result = ZRPC_ATOM_ACQUIRE_CAS(&bs->lock, &e, ZRPC_LFDS_FLAG_RAISED);
        if (result == 1) {
            /* TRD : if E[1] is less than 1/100th of E[0], decrease the metric, to increase E[1] */
            if (bs->backoff_iteration_frequency_counters[1] < bs->backoff_iteration_frequency_counters[0] / 100) {
                if (bs->metric >= 11)
                    bs->metric -= 10;
            } else
                bs->metric += 10;

            bs->backoff_iteration_frequency_counters[0] = 0;
            bs->backoff_iteration_frequency_counters[1] = 0;
            bs->total_operations = 0;

            ZRPC_ATOM_RELEASE_STORE(&bs->lock, ZRPC_LFDS_FLAG_LOWERED);
        }
    }
}

static void inline zRPC_lfds_misc_internal_backoff_init(struct zRPC_lfds_backoff_state *bs) {
    assert(bs != NULL);
    assert((uintptr_t) &bs->lock % ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES == 0);

    bs->lock = ZRPC_LFDS_FLAG_LOWERED;
    bs->backoff_iteration_frequency_counters[0] = 0;
    bs->backoff_iteration_frequency_counters[1] = 0;
    bs->metric = 1;
    bs->total_operations = 0;
}

#ifdef __cplusplus
}
#endif
#endif //ZRPC_LF_BACKOFF_H
