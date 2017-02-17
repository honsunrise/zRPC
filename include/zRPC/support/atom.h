//
// Created by zhsyourai on 11/26/16.
//

#ifndef ZRPC_ATOM_H
#define ZRPC_ATOM_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#if(defined __GNUC__ )

#if (defined __arm__ || defined __i686__ || defined __i586__ || defined __i486__ )

#define OPERATION_TYPE int long unsigned
#define OPERATION_TYPE_LONG int long long unsigned

#elif (defined __x86_64__)

#define OPERATION_TYPE int long long unsigned
#define OPERATION_TYPE_LONG OPERATION_TYPE

#endif

#define GCC_VERSION ( __GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__ )

#if(GCC_VERSION >= 412 && GCC_VERSION < 473)

static inline void __barrier_compiler(void) {
    __asm__ __volatile__ ( "" : : : "memory" );
}

#define ZRPC_ATOM_BARRIER_COMPILER_LOAD    __barrier_compiler()
#define ZRPC_ATOM_BARRIER_COMPILER_STORE   __barrier_compiler()
#define ZRPC_ATOM_BARRIER_COMPILER_FULL    __barrier_compiler()

#define ZRPC_ATOM_BARRIER_PROCESSOR_LOAD   __sync_synchronize()
#define ZRPC_ATOM_BARRIER_PROCESSOR_STORE  __sync_synchronize()
#define ZRPC_ATOM_BARRIER_PROCESSOR_FULL   __sync_synchronize()
/*
 * These function for atomic mem barrier operator
 */

#define ZRPC_ATOM_FULL_BARRIER()     (ZRPC_ATOM_BARRIER_COMPILER_FULL, ZRPC_ATOM_BARRIER_PROCESSOR_FULL)
#define ZRPC_ATOM_STORE_BARRIER()    (ZRPC_ATOM_BARRIER_COMPILER_FULL, ZRPC_ATOM_BARRIER_PROCESSOR_FULL)
#define ZRPC_ATOM_LOAD_BARRIER()     (ZRPC_ATOM_BARRIER_COMPILER_FULL, ZRPC_ATOM_BARRIER_PROCESSOR_FULL)

/*
 * These function for atomic load and store operator
 */
#define ZRPC_ATOM_ACQUIRE_LOAD(p)                                                 \
    ({                                                                            \
        typeof(*(p)) r;                                                           \
        *((OPERATION_TYPE*)&r) = __sync_fetch_and_add((OPERATION_TYPE*)(p), 0);   \
        ZRPC_ATOM_LOAD_BARRIER();                                                 \
        r;                                                                        \
    })

#define ZRPC_ATOM_NO_BARRIER_LOAD(p)                                            \
    ({                                                                            \
        typeof(*(p)) r;                                                           \
        *((OPERATION_TYPE*)&r) = __sync_fetch_and_add((OPERATION_TYPE*)(p), 0);   \
        r;                                                                        \
    })

#define ZRPC_ATOM_RELEASE_STORE(p, value)                                                \
    ({                                                                                   \
        typeof(value) v = value;                                                         \
        OPERATION_TYPE c = *((OPERATION_TYPE*)(p));                                      \
        ZRPC_ATOM_STORE_BARRIER();                                                       \
        __sync_bool_compare_and_swap((OPERATION_TYPE*)(p), c, *((OPERATION_TYPE*)(&v))); \
    })

#define ZRPC_ATOM_NO_BARRIER_STORE(p, value)                                             \
    ({                                                                                   \
        typeof(value) v = value;                                                         \
        OPERATION_TYPE c = *((OPERATION_TYPE*)(p));                                      \
        __sync_bool_compare_and_swap((OPERATION_TYPE*)(p), c, *((OPERATION_TYPE*)(&v))); \
    })

/*
 * These function for atomic fetch add operator
 */
#define ZRPC_ATOM_NO_BARRIER_FETCH_ADD(p, delta) __sync_fetch_and_add((p), (delta))

#define ZRPC_ATOM_FULL_FETCH_ADD(p, delta)                                                             \
    ({                                                                                                 \
        typeof(delta) v = delta;                                                                       \
        typeof(*p) r = *p;                                                                             \
        *((OPERATION_TYPE*)&r) = __sync_fetch_and_add((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&v))); \
        ZRPC_ATOM_FULL_BARRIER();                                                                      \
        r;                                                                                             \
    })

/*
 * These function for atomic add fetch operator
 */
#define ZRPC_ATOM_NO_BARRIER_ADD_FETCH(p, delta) __sync_add_and_fetch((p), (delta))

#define ZRPC_ATOM_FULL_ADD_FETCH(p, delta)                                                             \
    ({                                                                                                 \
        typeof(delta) v = delta;                                                                       \
        typeof(*p) r = *p;                                                                             \
        *((OPERATION_TYPE*)&r) = __sync_add_and_fetch((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&v))); \
        ZRPC_ATOM_FULL_BARRIER();                                                                      \
        r;                                                                                             \
    })

/*
 * These function for atomic fetch sub operator
 */
#define ZRPC_ATOM_NO_BARRIER_FETCH_SUB(p, delta) __sync_fetch_and_sub((p), (delta))

#define ZRPC_ATOM_FULL_FETCH_SUB(p, delta)                                                             \
    ({                                                                                                 \
        typeof(delta) v = delta;                                                                       \
        typeof(*p) r = *p;                                                                             \
        *((OPERATION_TYPE*)&r) = __sync_fetch_and_sub((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&v))); \
        ZRPC_ATOM_FULL_BARRIER();                                                                      \
        r;                                                                                             \
    })
/*
 * These function for atomic sub fetch operator
 */
#define ZRPC_ATOM_NO_BARRIER_SUB_FETCH(p, delta) __sync_sub_and_fetch((p), (delta))

#define ZRPC_ATOM_FULL_SUB_FETCH(p, delta)                                                             \
    ({                                                                                                 \
        typeof(delta) v = delta;                                                                       \
        typeof(*p) r = *p;                                                                             \
        *((OPERATION_TYPE*)&r) = __sync_sub_and_fetch((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&v))); \
        ZRPC_ATOM_FULL_BARRIER();                                                                      \
        r;                                                                                             \
    })


/*
 * These function for atomic cas and dwcas operator
 */

#define ZRPC_ATOM_NO_BARRIER_CAS(p, e, n)                                                                           \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(*e) v;                                                                                               \
        o = *(e);                                                                                                   \
        *((OPERATION_TYPE*)(e)) =                                                                                   \
            __sync_val_compare_and_swap((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&o)), *((OPERATION_TYPE*)(&v)));  \
        o == *(e);                                                                                                  \
    })

#define ZRPC_ATOM_ACQUIRE_CAS(p, e, n)                                                                              \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(*e) v;                                                                                               \
        o = *(e);                                                                                                   \
        *((OPERATION_TYPE*)(e)) =                                                                                   \
            __sync_val_compare_and_swap((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&o)), *((OPERATION_TYPE*)(&v)));  \
        ZRPC_ATOM_LOAD_BARRIER();                                                                                   \
        o == *(e);                                                                                                  \
    })


#define ZRPC_ATOM_RELEASE_CAS(p, e, n)                                                                              \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(*e) v;                                                                                               \
        o = *(e);                                                                                                   \
        ZRPC_ATOM_STORE_BARRIER();                                                                                  \
        *((OPERATION_TYPE*)(e)) =                                                                                   \
            __sync_val_compare_and_swap((OPERATION_TYPE*)(p), *((OPERATION_TYPE*)(&o)), *((OPERATION_TYPE*)(&v)));  \
        o == *(e);                                                                                                  \
    })

#if(defined __arm__ || defined __i686__ || defined __i586__ || defined __i486__ )
#define ZRPC_ATOM_NO_BARRIER_DWCAS(p, e, n)                                                                         \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(n) v = n;                                                                                            \
        o = *(e);                                                                                                   \
        *((OPERATION_TYPE_LONG*)(e)) =                                                                              \
            __sync_val_compare_and_swap((OPERATION_TYPE_LONG*)(p), *((OPERATION_TYPE_LONG*)(&o)),                   \
                                        *((OPERATION_TYPE_LONG*)(&v)));                                             \
        o == *(e);                                                                                                  \
    })

#define ZRPC_ATOM_ACQUIRE_DWCAS(p, e, n)                                                                            \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(n) v = n;                                                                                            \
        o = *(e);                                                                                                   \
        *((OPERATION_TYPE_LONG*)(e)) =                                                                              \
            __sync_val_compare_and_swap((OPERATION_TYPE_LONG*)(p), *((OPERATION_TYPE_LONG*)(&o)),                   \
                                        *((OPERATION_TYPE_LONG*)(&v)));                                             \
        ZRPC_ATOM_LOAD_BARRIER();                                                                                   \
        o == *(e);                                                                                                  \
    })

#define ZRPC_ATOM_RELEASE_DWCAS(p, e, n)                                                                            \
    ({                                                                                                              \
        typeof(*e) o;                                                                                               \
        typeof(n) v = n;                                                                                            \
        o = *(e);                                                                                                   \
        ZRPC_ATOM_STORE_BARRIER();                                                                                  \
        *((OPERATION_TYPE_LONG*)(e)) =                                                                              \
            __sync_val_compare_and_swap((OPERATION_TYPE_LONG*)(p), *((OPERATION_TYPE_LONG*)(&o)),                   \
                                        *((OPERATION_TYPE_LONG*)(&v)));                                             \
        o == *(e);                                                                                                  \
    })
#endif

#if(defined __x86_64__)
#define __ASM_CMPXCHG_(p, e, v, result)                                                                            \
    {                                                                                                               \
        result = 0;                                                                                                 \
        __asm__ __volatile__                                                                                        \
        (                                                                                                           \
            "lock;"                                                                                                 \
            "cmpxchg16b %0;"                                                                                        \
            "setz       %4;"                                                                                        \
            : "+m" ((OPERATION_TYPE_LONG*)(p)[0]), "+m" ((OPERATION_TYPE_LONG*)(p)[1]),                             \
              "+a" ((OPERATION_TYPE_LONG*)(e)[0]), "+d" ((OPERATION_TYPE_LONG*)(e)[1]), "=q" (result)               \
            : "b" ((OPERATION_TYPE_LONG*)(v)[0]), "c" ((OPERATION_TYPE_LONG*)(v)[1])                                \
            :                                                                                                       \
        )                                                                                                           \
    }

#define ZRPC_ATOM_NO_BARRIER_DWCAS(p, e, n)                                                                            \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          typeof(e) ev = e;                                                                                            \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(&ev), (OPERATION_TYPE_LONG*)(&nv), result); \
          result;                                                                                                      \
    })

#define ZRPC_ATOM_ACQUIRE_DWCAS(p, e, n)                                                                               \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          typeof(e) ev = e;                                                                                            \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(&ev), (OPERATION_TYPE_LONG*)(&nv), result); \
          ZRPC_ATOM_LOAD_BARRIER();                                                                                    \
          result;                                                                                                      \
    })

#define ZRPC_ATOM_RELEASE_DWCAS(p, e, n)                                                                               \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          typeof(e) ev = e;                                                                                            \
          ZRPC_ATOM_STORE_BARRIER();                                                                                   \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(&ev), (OPERATION_TYPE_LONG*)(&nv), result); \
          result;                                                                                                      \
    })
#endif

#elif(GCC_VERSION >= 473)

#define ZRPC_ATOM_BARRIER_PROCESSOR_LOAD   __atomic_thread_fence( __ATOMIC_ACQUIRE )
#define ZRPC_ATOM_BARRIER_PROCESSOR_STORE  __atomic_thread_fence( __ATOMIC_RELEASE )
#define ZRPC_ATOM_BARRIER_PROCESSOR_FULL   __atomic_thread_fence( __ATOMIC_ACQ_REL )

/*
 * These function for atomic mem barrier operator
 */

#define ZRPC_ATOM_FULL_BARRIER()     ZRPC_ATOM_BARRIER_PROCESSOR_LOAD
#define ZRPC_ATOM_STORE_BARRIER()    ZRPC_ATOM_BARRIER_PROCESSOR_STORE
#define ZRPC_ATOM_LOAD_BARRIER()     ZRPC_ATOM_BARRIER_PROCESSOR_FULL

/*
 * These function for atomic load and store operator
 */

#define ZRPC_ATOM_ACQUIRE_LOAD(p)                  __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define ZRPC_ATOM_NO_BARRIER_LOAD(p)               __atomic_load_n((p), __ATOMIC_RELAXED)
#define ZRPC_ATOM_RELEASE_STORE(p, value)          __atomic_store_n((p), (value), __ATOMIC_RELEASE)
#define ZRPC_ATOM_NO_BARRIER_STORE(p, value)       __atomic_store_n((p), (value), __ATOMIC_RELAXED)

/*
 * These function for atomic fetch add operator
 */

#define ZRPC_ATOM_NO_BARRIER_FETCH_ADD(p, delta)       __atomic_fetch_add((p), (delta), __ATOMIC_RELAXED)
#define ZRPC_ATOM_FULL_FETCH_ADD(p, delta)             __atomic_fetch_add((p), (delta), __ATOMIC_ACQ_REL)

/*
 * These function for atomic add fetch operator
 */

#define ZRPC_ATOM_NO_BARRIER_ADD_FETCH(p, delta)       __atomic_add_fetch((p), (delta), __ATOMIC_RELAXED)
#define ZRPC_ATOM_FULL_ADD_FETCH(p, delta)             __atomic_add_fetch((p), (delta), __ATOMIC_ACQ_REL)

/*
 * These function for atomic fetch add operator
 */

#define ZRPC_ATOM_NO_BARRIER_FETCH_SUB(p, delta)       __atomic_fetch_sub((p), (delta), __ATOMIC_RELAXED)
#define ZRPC_ATOM_FULL_FETCH_SUB(p, delta)             __atomic_fetch_sub((p), (delta), __ATOMIC_ACQ_REL)

/*
 * These function for atomic sub fetch operator
 */

#define ZRPC_ATOM_NO_BARRIER_SUB_FETCH(p, delta)       __atomic_sub_fetch((p), (delta), __ATOMIC_RELAXED)
#define ZRPC_ATOM_FULL_SUB_FETCH(p, delta)             __atomic_sub_fetch((p), (delta), __ATOMIC_ACQ_REL)

/*
 * These function for atomic cas operator
 */

#define ZRPC_ATOM_NO_BARRIER_CAS(p, e, n)                                                  \
    ({                                                                                     \
        __atomic_compare_exchange_n((p), (e), (n), 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
    })

#define ZRPC_ATOM_ACQUIRE_CAS(p, e, n)                                                     \
    ({                                                                                     \
        __atomic_compare_exchange_n((p), (e), (n), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); \
    })

#define ZRPC_ATOM_RELEASE_CAS(p, e, n)                                                     \
    ({                                                                                     \
        __atomic_compare_exchange_n((p), (e), (n), 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED); \
    })


#if(defined __arm__ || defined __i686__ || defined __i586__ || defined __i486__ )
#define ZRPC_ATOM_NO_BARRIER_DWCAS(p, o, n) \
    __atomic_compare_exchange_n( (int long long unsigned *) (p), (int long long unsigned *) &(o), *(int long long unsigned *) (n), 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED );
#endif

#if(defined __x86_64__ )
#define __ASM_CMPXCHG_(p, e, v, result)                                                                             \
    {                                                                                                               \
        result = 0;                                                                                                 \
        __asm__ __volatile__                                                                                        \
        (                                                                                                           \
            "lock;"                                                                                                 \
            "cmpxchg16b %0;"                                                                                        \
            "setz       %4;"                                                                                        \
            : "+m" ((OPERATION_TYPE_LONG*)(p)[0]), "+m" ((OPERATION_TYPE_LONG*)(p)[1]),                             \
              "+a" ((OPERATION_TYPE_LONG*)(e)[0]), "+d" ((OPERATION_TYPE_LONG*)(e)[1]), "=q" (result)               \
            : "b" ((OPERATION_TYPE_LONG*)(v)[0]), "c" ((OPERATION_TYPE_LONG*)(v)[1])                                \
            :                                                                                                       \
        );                                                                                                          \
    }

#define ZRPC_ATOM_NO_BARRIER_DWCAS(p, e, n)                                                                            \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(e), (OPERATION_TYPE_LONG*)(&nv), result); \
          result;                                                                                                      \
    })

#define ZRPC_ATOM_ACQUIRE_DWCAS(p, e, n)                                                                               \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(e), (OPERATION_TYPE_LONG*)(&nv), result); \
          ZRPC_ATOM_LOAD_BARRIER();                                                                                    \
          result;                                                                                                      \
    })

#define ZRPC_ATOM_RELEASE_DWCAS(p, e, n)                                                                               \
    ({                                                                                                                 \
          unsigned char result;                                                                                        \
          typeof(n) nv = n;                                                                                            \
          ZRPC_ATOM_STORE_BARRIER();                                                                                   \
          __ASM_CMPXCHG_((OPERATION_TYPE_LONG*)(p), (OPERATION_TYPE_LONG*)(e), (OPERATION_TYPE_LONG*)(&nv), result); \
          result;                                                                                                      \
    })
#endif

#endif

#if(defined __arm__ )

#ifdef ZRPC_ATOM_ARCH_PROCESSOR
#error More than one porting option matches the current architecture.
#endif

#define ZRPC_ATOM_ARCH_PROCESSOR                   32
#define ZRPC_ATOM_ARCH_PROCESSOR_STRING            "ARM (32-bit)"
#define ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES          2048

#endif


#if(defined __aarch64__ )

#ifdef ZRPC_ATOM_ARCH_PROCESSOR
#error More than one porting option matches the current architecture.
#endif

#define ZRPC_ATOM_ARCH_PROCESSOR                   64
#define ZRPC_ATOM_ARCH_PROCESSOR_STRING            "ARM (64-bit)"
#define ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES          2048

#endif


#if(defined __i686__ || defined __i586__ || defined __i486__)

#ifdef ZRPC_ATOM_ARCH_PROCESSOR
#error More than one porting option matches the current architecture.
#endif

#define ZRPC_ATOM_ARCH_PROCESSOR                   32
#define ZRPC_ATOM_ARCH_PROCESSOR_STRING            "x86"
#define ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES          32

#endif


#if(defined __x86_64__ )

#ifdef ZRPC_ATOM_ARCH_PROCESSOR
#error More than one porting option matches the current architecture.
#endif

#define ZRPC_ATOM_ARCH_PROCESSOR                   64
#define ZRPC_ATOM_ARCH_PROCESSOR_STRING            "x64"
#define ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES          128

#endif


#if(defined __ia64__ )

#ifdef ZRPC_ATOM_ARCH_PROCESSOR
#error More than one porting option matches the current architecture.
#endif

#define ZRPC_ATOM_ARCH_PROCESSOR                   64
#define ZRPC_ATOM_ARCH_PROCESSOR_STRING            "IA64"
#define ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES          64

#endif
#endif


#if(!defined ZRPC_ATOM_ARCH_PROCESSOR )
#error No matching architecture.
#else
#if ZRPC_ATOM_ARCH_PROCESSOR == 32

#define ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH        4
#define ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD_LENGTH        8

#elif ZRPC_ATOM_ARCH_PROCESSOR == 64

#define ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH        8
#define ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD_LENGTH        16

#else
#error No matching machine word-length.
#endif
#endif

#define ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD               __attribute__((aligned(ZRPC_ATOM_ARCH_ALIG_SINGLE_WORD_LENGTH)))
#define ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD               __attribute__((aligned(ZRPC_ATOM_ARCH_ALIG_DOUBLE_WORD_LENGTH)))
#define ZRPC_ATOM_ARCH_ALIG_CACHELINE                 __attribute__((aligned(ZRPC_ATOM_ARCH_CACHELINE_IN_BYTES)))

typedef struct tagged_pointer {
    void *ptr;
    uintptr_t tag;
}tagged_pointer;

#define TAGGED_POINT_COMPARE(p, q) (p.ptr == q.ptr && p.tag == q.tag)

//typedef union zRPC_atom {
//    uintptr_t uint_v;
//    intptr_t int_v;
//    struct {
//        uintptr_t dw_0;
//        uintptr_t dw_1;
//    };
//    struct tagged_pointer tp_v;
//} zRPC_atom;

typedef uintptr_t zRPC_atom;

#ifdef __cplusplus
}
#endif
#endif //ZRPC_ATOM_H
