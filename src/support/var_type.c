//
// Created by zhsyourai on 1/31/17.
//

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "zRPC/support/var_type.h"

DEFINE_REFERENCE_FUN(zRPC_value)

static void release_var_function(void *ptr) {
    zRPC_value *value = ptr;
    switch (value->type) {
        case STR:
            free(value->str_value.str);
            break;
        case ARRAY:
            for (int i = 0; i < value->array_value->len; ++i) {
                release_var_function(value->array_value->value[i]);
            }
            free(value->array_value->value);
            free(value->array_value);
            break;
        case MAP:
            for (int i = 0; i < value->map_value->len; ++i) {
                if (value->map_value->value[i].key->type == STR) {
                    release_var_function(value->map_value->value[i].key);
                }
                if (value->map_value->value[i].value->type == STR) {
                    release_var_function(value->map_value->value[i].value);
                }
            }
            free(value->map_value->value);
            free(value->map_value);
            break;
        default:
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

zRPC_value *zRPC_type_var_create_base(zRPC_TYPE type, const void *value) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_var_function);
    ret_value->type = type;
    switch (type) {
        case BYTE:
            ret_value->type = BYTE;
            ret_value->byte_value = *(int8_t *) value;
            break;
        case INT64:
            ret_value->type = INT64;
            ret_value->int64_value = *(int64_t *) value;
            break;
        case FLOAT:
            ret_value->type = FLOAT;
            ret_value->float_value = *(float *) value;
            break;
        case STR:
            ret_value->type = STR;
            ret_value->str_value.len = strlen((char *) value);
            ret_value->str_value.str = malloc(ret_value->str_value.len + 1);
            ret_value->str_value.str[ret_value->str_value.len] = '\0';
            strcpy(ret_value->str_value.str, (char *) value);
            return ret_value;
        default:
            exit(-1);
    }
    return ret_value;
}

zRPC_value *zRPC_type_var_create_array(size_t size) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_var_function);
    ret_value->array_value = malloc(sizeof(zRPC_array_value));
    ret_value->type = ARRAY;
    ret_value->array_value->value = (zRPC_value **) calloc(size, sizeof(zRPC_value*));
    ret_value->array_value->len = size;
    return ret_value;
}

zRPC_value *zRPC_type_var_create_map(size_t size) {
    zRPC_value *ret_value = malloc(sizeof(zRPC_value));
    INIT_REFERENCE(ret_value, release_var_function);
    ret_value->map_value = malloc(sizeof(zRPC_map_value));
    ret_value->type = MAP;
    ret_value->map_value->value = (zRPC_map_entry *) calloc(size, sizeof(zRPC_map_entry));
    ret_value->map_value->len = size;
    return ret_value;
}

void zRPC_type_var_destory(zRPC_value *value) {
    release_var_function(value);
}

