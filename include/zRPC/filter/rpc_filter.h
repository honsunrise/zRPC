//
// Created by zhsyourai on 2/16/17.
//

#ifndef ZRPC_RPC_FILTER_H
#define ZRPC_RPC_FILTER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "zRPC/rpc/call.h"

typedef void (*call_function)(void *param, const char*name,
                              zRPC_caller_instance *caller_instance, zRPC_call *call, zRPC_call_result *result);

zRPC_filter_factory *rpc_filter_factory(call_function function, void *param);

void rpc_filter_on_active(zRPC_filter *filter, zRPC_channel *channel);

void rpc_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out);

void rpc_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out);

void rpc_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_RPC_FILTER_H
