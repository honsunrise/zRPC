//
// Created by zhsyourai on 2/10/17.
//

#ifndef ZRPC_LF_LIST_H
#define ZRPC_LF_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "../lf_base.h"
#include "../lf_backoff.h"

struct zRPC_list_element {
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD next;
    void ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD *value;
};

struct zRPC_list_state {
    struct zRPC_list_element ZRPC_ATOM_ARCH_ALIG_CACHELINE dummy_element;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE end;
    struct tagged_pointer ZRPC_ATOM_ARCH_ALIG_CACHELINE start;
    struct zRPC_lfds_backoff_state after_backoff, end_backoff, start_backoff;
};

void zRPC_list_init(struct zRPC_list_state *l);


void zRPC_list_insert_at_start(struct zRPC_list_state *l,
                               struct zRPC_list_element *e);

void zRPC_list_insert_at_end(struct zRPC_list_state *l,
                             struct zRPC_list_element *e);

void zRPC_list_insert_after_element(struct zRPC_list_state *l,
                                    struct zRPC_list_element *e,
                                    struct zRPC_list_element *e_pre);

size_t zRPC_list_count(struct zRPC_list_state *l);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_LF_LIST_H
