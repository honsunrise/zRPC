//
// Created by zhsyourai on 12/1/16.
//

#ifndef ZRPC_LITEPACKAGE_H
#define ZRPC_LITEPACKAGE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "zRPC/channel.h"

typedef struct zRPC_litepackage_custom zRPC_litepackage_custom;

zRPC_filter_factory *litepackage_filter_factory();

void litepackage_filter_on_active(zRPC_filter *filter, zRPC_channel *channel, void *tag);
void litepackage_filter_on_inactive(zRPC_filter *filter, zRPC_channel *channel, void *tag);
void
litepackage_filter_on_readable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);
void
litepackage_filter_on_writable(zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out, void *tag);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_LITEPACKAGE_H
