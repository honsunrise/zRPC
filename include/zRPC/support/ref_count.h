//
// Created by zhsyourai on 1/31/17.
//

#ifndef ZRPC_REF_COUNT_H
#define ZRPC_REF_COUNT_H

#include "runnable.h"
#include "useful.h"
#include "atom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*release_function_t)(void *ptr);

struct reference_struct_t {
    zRPC_atom count;
    release_function_t release_function;
};

#define DECLARE_REFERENCE \
    struct reference_struct_t __ref__

#define INIT_REFERENCE(ptr, r_f) \
    ({ \
        ZRPC_ATOM_RELEASE_STORE(&ptr->__ref__.count, 1); \
        ptr->__ref__.release_function = r_f; \
    })

#define DECLARE_REFERENCE_FUN(type) \
    zRPC_atom __##type##__add_ref__(struct type *ptr); \
    zRPC_atom __##type##__sub_ref__(struct type *ptr); \
    type *__##type##__pass_ref__(struct type *ptr);

#define DEFINE_REFERENCE_FUN(type) \
    zRPC_atom __##type##__add_ref__(struct type *ptr) { \
        return ZRPC_ATOM_FULL_ADD_FETCH(&ptr->__ref__.count, 1); \
    } \
    zRPC_atom __##type##__sub_ref__(struct type *ptr) { \
        if(!ZRPC_ATOM_FULL_SUB_FETCH(&ptr->__ref__.count, 1)) { \
            ptr->__ref__.release_function(ptr); \
        } \
        return ZRPC_ATOM_ACQUIRE_LOAD(&ptr->__ref__.count); \
    } \
    type *__##type##__pass_ref__(struct type *ptr) { \
        ZRPC_ATOM_FULL_ADD_FETCH(&ptr->__ref__.count, 1); \
        return ptr; \
    }

#define DEINIT_REFERENCE(ptr) \
    ({ \
        ZRPC_ATOM_RELEASE_STORE(&ptr->__ref__.count, 0); \
        ptr->__ref__.release_function(container_of(ptr, type, __ref__)); \
        ptr->__ref__.release_function = NULL; \
    })

#define ADD_REFERENCE(ptr, type) \
        __##type##__add_ref__(ptr)

#define SUB_REFERENCE(ptr, type) \
        __##type##__sub_ref__(ptr)

#define PASS_PTR(ptr, type) \
        __##type##__pass_ref__(ptr)

#ifdef __cplusplus
}
#endif
#endif //ZRPC_REF_COUNT_H
