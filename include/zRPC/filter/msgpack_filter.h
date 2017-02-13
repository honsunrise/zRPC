//
// Created by zhsyourai on 11/27/16.
//

#ifndef ZRPC_MSGPACK_FILTER_H
#define ZRPC_MSGPACK_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/channel.h"

zRPC_filter_factory *msgpack_filter_factory();

void msgpack_filter_on_active(zRPC_filter *filter, zRPC_channel *channel, void *tag);
void msgpack_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel, void *tag);
void msgpack_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);
void msgpack_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_MSGPACK_FILTER_H
