//
// Created by zhsyourai on 1/31/17.
//

#ifndef ZRPC_VAR_TYPE_H
#define ZRPC_VAR_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "ref_count.h"

typedef enum zRPC_TYPE {
    BYTE = 0x01, INT64 = 0x02, FLOAT = 0x04, STR = 0x08, ARRAY = 0x10, MAP = 0x20
} zRPC_TYPE;

typedef struct zRPC_str_value {
    char *str;
    size_t len;
} zRPC_str_value;

typedef struct zRPC_value zRPC_value;

typedef struct zRPC_array_value {
    zRPC_value **value;
    size_t len;
} zRPC_array_value;

typedef struct zRPC_map_entry {
    zRPC_value *key;
    zRPC_value *value;
} zRPC_map_entry;

typedef struct zRPC_map_value {
    zRPC_map_entry *value;
    size_t len;
} zRPC_map_value;

struct zRPC_value {
    DECLARE_REFERENCE;
    zRPC_TYPE type;
    union {
        int8_t byte_value;
        int32_t int32_value;
        uint32_t uint32_value;
        int64_t int64_value;
        uint64_t uint64_value;
        float float_value;
        double double_value;
        zRPC_str_value str_value;
        zRPC_array_value *array_value;
        zRPC_map_value *map_value;
    };
};

DECLARE_REFERENCE_FUN(zRPC_value)

zRPC_str_value zRPC_type_create_str(const char *str);

/*
 * For var type
 */

zRPC_value *zRPC_type_var_create_base(zRPC_TYPE type, const void *value);

zRPC_value *zRPC_type_var_create_array(size_t size);

zRPC_value *zRPC_type_var_create_map(size_t size);

void zRPC_type_var_destory(zRPC_value *value);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_VAR_TYPE_H
