//
// Created by zhsyourai on 11/25/16.
//

#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include "support/string_utils.h"

char *zRPC_str_dup(const char *src) {
    char *dst;
    size_t len;

    if (!src) {
        return NULL;
    }

    len = strlen(src) + 1;
    dst = malloc(len);

    memcpy(dst, src, len);

    return dst;
}

int zRPC_sprintf_out(char **out, const char *format, ...) {
    va_list args;
    int ret;
    char buf[64];
    size_t strp_buflen;

    /* Use a constant-sized buffer to determine the length. */
    va_start(args, format);
    ret = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    if (ret < 0) {
        *out = NULL;
        return -1;
    }

    /* Allocate a new buffer, with space for the NUL terminator. */
    strp_buflen = (size_t) ret + 1;
    if ((*out = malloc(strp_buflen)) == NULL) {
        /* This shouldn't happen, because gpr_malloc() calls abort(). */
        return -1;
    }

    /* Return early if we have all the bytes. */
    if (strp_buflen <= sizeof(buf)) {
        memcpy(*out, buf, strp_buflen);
        return ret;
    }

    /* Try again using the larger buffer. */
    va_start(args, format);
    ret = vsnprintf(*out, strp_buflen, format, args);
    va_end(args);
    if ((size_t) ret == strp_buflen - 1) {
        return ret;
    }

    /* This should never happen. */
    free(*out);
    *out = NULL;
    return -1;
}
