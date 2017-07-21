//
// Created by zhswo on 2016/11/23.
//

#ifndef ZRPC_CHANNEL_H
#define ZRPC_CHANNEL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "event.h"
#include "filter.h"
#include "scheduling.h"

struct zRPC_filter;

struct zRPC_filter_factory;

typedef struct zRPC_pipe zRPC_pipe;

typedef struct zRPC_channel zRPC_channel;

void zRPC_pipe_create(zRPC_pipe **out);

void zRPC_pipe_destroy(zRPC_pipe *pipe);

void zRPC_pipe_add_filter_with_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory *filter_factory);

void zRPC_pipe_add_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory);

void zRPC_pipe_remove_fileter_by_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory **filter_factory);

void zRPC_pipe_remove_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory);

void zRPC_channel_create(zRPC_channel **out, zRPC_pipe *pipe, zRPC_fd *fd, zRPC_scheduler *context);

void zRPC_channel_destroy(zRPC_channel *channel);

void zRPC_channel_set_custom_data(zRPC_channel *channel, void *custom_data);

void *zRPC_channel_get_custom_data(zRPC_channel *channel);

zRPC_fd *zRPC_channel_get_fd(zRPC_channel *channel);

void *zRPC_channel_on_active(zRPC_channel *channel);

void *zRPC_channel_on_read(zRPC_channel *channel);

void *zRPC_channel_on_write(zRPC_channel *channel);

void *zRPC_channel_on_inactive(zRPC_channel *channel);

void zRPC_channel_write(zRPC_channel *channel, void *msg);


void zRPC_channel_event_on_read(zRPC_channel *channel);

void zRPC_channel_event_on_write(zRPC_channel *channel);

#ifdef __cplusplus
}
#endif
#endif //ZRPC_CHANNEL_H
