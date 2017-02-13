//
// Created by zhsyourai on 1/31/17.
//

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "zRPC/support/var_type.h"

DEFINE_REFERENCE_FUN(zRPC_base_value)

DEFINE_REFERENCE_FUN(zRPC_array_value)

DEFINE_REFERENCE_FUN(zRPC_map_value)

DEFINE_REFERENCE_FUN(zRPC_value)

static void release_base_function(void *ptr) {
    zRPC_base_value *value = ptr;
    if (value->type == STR) {
        free(value->value.str_value.str);
    }
    free(value);
}

static void release_array_function(void *ptr) {
    zRPC_array_value *value = ptr;
    for (int i = 0; i < value->len; ++i) {
        if (value->type == STR) {
            free(value->value[i].str_value.str);
        }
    }
    free(value);
}

static void release_map_function(void *ptr) {
    zRPC_map_value *value = ptr;
    for (int i = 0; i < value->len; ++i) {
        if (value->value[i].key->type == STR) {
            free(value->value[i].key->value.str_value.str);
        }
        if (value->value[i].value->type == STR) {
            free(value->value[i].value->value.str_value.str);
        }
    }
    free(value);
}

static void release_var_function(void *ptr) {
    zRPC_value *value = ptr;
    switch (value->type) {
        case BASE:
            release_base_function(value->base_value);
            break;
        case ARRAY:
            release_array_function(value->array_value);
            break;
        case MAP:
            release_map_function(value->map_value);
            break;
    }
    free(value);
}

zRPC_str_value zRPC_type_create_str(const char *str) {
    zRPC_str_value ret_value;
    ret_value.len = strlen(str);
    ret_value.str = malloc(ret_value.len + 1);
    ret_value.str[ret_value.len] = '\0';
    strcpy(ret_value.str, str);
    return ret_value;
}

zRPC_base_value *zRPC_type_base_create(zRPC_BASE_TYPE type, const void *value) {
    zRPC_base_value *ret_value;
    ret_value = malloc(sizeof(zRPC_base_value));
    INIT_REFERENCE(ret_value, release_base_function);
    switch (type) {
        case BYTE:
            ret_value->type = BYTE;
            ret_value->value.byte_value = *(int8_t *) value;
            break;
        case INT64:
            ret_value->type = INT64;
            ret_value->value.int64_value = *(int64_t *) value;
            break;
        case FLOAT:
            ret_value->type = FLOAT;
            ret_value->value.float_value = *(float *) value;
            break;
        case STR:
            ret_value->type = STR;
            ret_value->value.str_value.len = strlen((char *) value);
            ret_value->value.str_value.str = malloc(ret_value->value.str_value.len + 1);
            ret_value->value.str_value.str[ret_value->value.str_value.len] = '\0';
            strcpy(ret_value->value.str_value.str, (char *) value);
            return ret_value;
        default:
            exit(-1);
    }
    return ret_value;
}

zRPC_value *zRPC_type_var_create_base(zRPC_base_value *value) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_var_function);
    ret_value->base_value = value;
    ret_value->type = BASE;
    return ret_value;
}

zRPC_value *zRPC_type_var_create_array(zRPC_BASE_TYPE type, size_t size) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_base_function);
    ret_value->array_value = malloc(sizeof(zRPC_array_value));
    INIT_REFERENCE(ret_value->array_value, release_array_function);
    ret_value->type = ARRAY;
    ret_value->array_value->value = (zRPC_base_union *) malloc(sizeof(zRPC_base_union) * size);
    ret_value->array_value->len = size;
    ret_value->array_value->type = type;
    return ret_value;
}

zRPC_value *zRPC_type_var_create_map(size_t size) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_base_function);
    ret_value->map_value = malloc(sizeof(zRPC_map_value));
    INIT_REFERENCE(ret_value->map_value, release_map_function);
    ret_value->type = MAP;
    ret_value->map_value->value = (zRPC_map_entry *) malloc(sizeof(zRPC_map_entry) * size);
    ret_value->map_value->len = size;
    return ret_value;
}

void zRPC_type_var_destory(zRPC_value *value) {
    release_base_function(value);
}

