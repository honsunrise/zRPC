//
// Created by zhsyourai on 12/13/16.
//

#ifndef ZRPC_BYTES_FILTER_H
#define ZRPC_BYTES_FILTER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/channel.h"

zRPC_filter_factory *bytes_filter_factory();

void
bytes_filter_on_active(zRPC_filter *filter, zRPC_channel *channel, void *tag);

void
bytes_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);

void
bytes_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);

void
bytes_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel, void *tag);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_BYTES_FILTER_H
