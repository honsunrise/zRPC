//
// Created by zhsyourai on 11/25/16.
//

#ifndef ZRPC_STRING_UTILS_H
#define ZRPC_STRING_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

char *zRPC_str_dup(const char *src);

int zRPC_sprintf_out(char **out, const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_STRING_UTILS_H
