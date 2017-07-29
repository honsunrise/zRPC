//
// Created by zhsyourai on 12/1/16.
//

#ifndef ZRPC_USEFUL_H
#define ZRPC_USEFUL_H

#ifndef MAX
#define MAX(a, b) \
    (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) \
    (((a) < (b)) ? (a) : (b))
#endif

#ifndef offset_of

#define offset_of(type, member) \
    (size_t)(&((type*)0)->member)

#endif

#define container_of(ptr, type, member) \
    ({ \
        const typeof(((type *)0)->member)*__mptr = (ptr); \
        (type *)((char *)__mptr - offset_of(type, member)); \
    })

#endif //ZRPC_USEFUL_H
