//
// Created by zhsyourai on 2/5/17.
//

#ifndef ZRPC_LF_STACK_H
#define ZRPC_LF_STACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "../lf_base.h"
#include "../lf_backoff.h"

struct zRPC_stack_element {
    struct zRPC_stack_element ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD *next;
    void ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD *value;
};

struct zRPC_stack_state {
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE top;

    struct zRPC_lfds_backoff_state pop_backoff, push_backoff;
};

void zRPC_stack_init(struct zRPC_stack_state *fs);


void zRPC_stack_cleanup(struct zRPC_stack_state *fs,
                        void (*element_cleanup_callback)(struct zRPC_stack_state *fs,
                                                         struct zRPC_stack_element *fe));

int zRPC_stack_pop(struct zRPC_stack_state *fs, struct zRPC_stack_element **fe);


void zRPC_stack_push(struct zRPC_stack_state *fs, struct zRPC_stack_element *fe);

size_t zRPC_stack_count(struct zRPC_stack_state *fs);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_LF_STACK_H
