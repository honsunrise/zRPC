//
// Created by zhswo on 2016/11/23.
//

#include <malloc.h>
#include <assert.h>
#include "rtti.h"
#include "channel.h"
#include "support/socket_utils.h"
#include "support/bytes_buf.h"

/* Call filter function */
void zRPC_filter_call_on_active(struct zRPC_filter *filter, zRPC_channel *channel);

void zRPC_filter_call_on_inactive(struct zRPC_filter *filter, zRPC_channel *channel);

void zRPC_filter_call_on_read(struct zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out);

void zRPC_filter_call_on_write(struct zRPC_filter *filter, zRPC_channel *channel, void *msg, zRPC_filter_out *out);

static void *_channel_on_active(zRPC_channel *channel);

static void *_channel_on_read(zRPC_channel *channel);

static void *_channel_on_write(zRPC_channel *channel);

static void *_channel_on_inactive(zRPC_channel *channel);

typedef struct zRPC_channel_write_param {
  size_t write_remained;
  size_t outgoing_buf_len;
  zRPC_bytes_buf *outgoing_buf;
  zRPC_runnable *write_callback;
  zRPC_list_head list_node_pending;
  zRPC_list_head list_node_done;
} zRPC_channel_write_param;

void zRPC_pipe_create(zRPC_pipe **out) {
  zRPC_pipe *pipe = (zRPC_pipe *) malloc(sizeof(zRPC_pipe));
  pipe->filters = 0;
  pipe->head = NULL;
  pipe->tail = NULL;
  *out = pipe;
}

void zRPC_pipe_destroy(zRPC_pipe *pipe) {
  if (pipe != NULL) {
    free(pipe);
  }
}

void zRPC_pipe_add_filter_with_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory *filter_factory) {
  zRPC_filter_factory_linked_node *filter_node = malloc(sizeof(zRPC_filter_factory_linked_node));
  filter_node->name = name;
  filter_node->filter_factory = filter_factory;
  filter_node->next = NULL;
  filter_node->prev = NULL;
  if (pipe->head == NULL) {
    pipe->head = filter_node;
  }
  if (pipe->tail != NULL) {
    pipe->tail->next = filter_node;
  }
  filter_node->prev = pipe->tail;
  pipe->tail = filter_node;
  pipe->filters++;
}

void zRPC_pipe_add_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory) {
  zRPC_filter_factory_linked_node *filter_node = malloc(sizeof(zRPC_filter_factory_linked_node));
  filter_node->name = "";
  filter_node->filter_factory = filter_factory;
  filter_node->next = NULL;
  filter_node->prev = NULL;
  if (pipe->head == NULL) {
    pipe->head = filter_node;
  }
  if (pipe->tail != NULL) {
    pipe->tail->next = filter_node;
  }
  filter_node->prev = pipe->tail;
  pipe->tail = filter_node;
  pipe->filters++;
}

void zRPC_pipe_remove_filter_by_name(zRPC_pipe *pipe, const char *name, struct zRPC_filter_factory **filter_factory) {
  pipe->filters--;
}

void zRPC_pipe_remove_filter(zRPC_pipe *pipe, struct zRPC_filter_factory *filter_factory) {
  pipe->filters--;
}

static void _event_on_read(zRPC_channel *channel) {
  zRPC_mutex_lock(&channel->read_lock);
  if (!channel->is_active) {
    channel->is_active = 1;
    _channel_on_active(channel);
  }
  _channel_on_read(channel);
  zRPC_mutex_unlock(&channel->read_lock);
}

static void _event_on_write(zRPC_channel *channel) {
  zRPC_mutex_lock(&channel->write_lock);
  if (!channel->is_active) {
    channel->is_active = 1;
    _channel_on_active(channel);
  }
  _channel_on_write(channel);
  zRPC_mutex_unlock(&channel->write_lock);
}

static void _event_listener_callback(void *source, zRPC_event event, void *param) {
  switch (event.event_type) {
    case EV_WRITE:_channel_on_write(source);
      break;
    case EV_READ:_channel_on_read(source);
      break;
    case EV_CLOSE:_channel_on_inactive(source);
      break;
    case EV_OPEN:_channel_on_active(source);
      break;
    case EV_ERROR:break;
    default:assert(0);
      break;
  }
}

static void _destroy_callback(struct zRPC_event_source *source) {
  zRPC_channel *channel = container_of(source, zRPC_channel, source);
  zRPC_channel_destroy(channel);
}

void zRPC_channel_create(zRPC_channel **out, zRPC_pipe *pipe, int fd, zRPC_scheduler *scheduler) {
  zRPC_channel *channel = (zRPC_channel *) malloc(sizeof(zRPC_channel));
  RTTI_INIT_PTR(zRPC_channel, &channel->source);
  zRPC_source_init(&channel->source);
  channel->source.destroy_callback = _destroy_callback;
  channel->pipe = pipe;
  channel->tail = channel->head = NULL;
  zRPC_filter_factory_linked_node *head = channel->pipe->head;
  while (head != NULL) {
    zRPC_filter_linked_node *filter_node = malloc(sizeof(zRPC_filter_linked_node));
    zRPC_filter_create_by_factory(&filter_node->filter, head->filter_factory);
    filter_node->next = NULL;
    filter_node->prev = NULL;
    if (channel->head == NULL) {
      channel->head = filter_node;
    }
    if (channel->tail != NULL) {
      channel->tail->next = filter_node;
    }
    filter_node->prev = channel->tail;
    channel->tail = filter_node;

    head = head->next;
  }
  channel->fd = fd;
  channel->scheduler = scheduler;
  channel->is_active = 0;
  channel->is_writing = 0;
  zRPC_mutex_init(&channel->write_lock);
  zRPC_mutex_init(&channel->read_lock);
  zRPC_list_init(&channel->pending_write);
  zRPC_list_init(&channel->done_write);
  zRPC_ring_buf_create(&channel->buffer, 4096);
  zRPC_scheduler_register_source(scheduler, &channel->source);
  channel->e_a = zRPC_source_register_listener(&channel->source,
                                EV_READ | EV_CLOSE | EV_ERROR,
                                0,
                                _event_listener_callback,
                                NULL);
  zRPC_source_register_listener(&channel->source,
                                EV_OPEN,
                                1,
                                _event_listener_callback,
                                NULL);
  *out = channel;
}

void zRPC_channel_destroy(zRPC_channel *channel) {
  if (channel != NULL) {
    zRPC_filter_linked_node *head = channel->head;
    zRPC_filter_linked_node *node;
    while (head != NULL) {
      node = head;
      zRPC_filter_destroy(node->filter);
      head = head->next;
      free(node);
    }
    zRPC_source_unregister_listener(&channel->source, channel->e_a);
    zRPC_scheduler_unregister_source(channel->scheduler, &channel->source);
    free(channel);
  }
}

void zRPC_channel_set_custom_data(zRPC_channel *channel, void *custom_data) {
  channel->custom_data = custom_data;
}

void *zRPC_channel_get_custom_data(zRPC_channel *channel) {
  return channel->custom_data;
}

static void help_call_read_filter(zRPC_filter_linked_node *filter, zRPC_channel *channel, void *msg) {
  zRPC_filter_out *out;
  zRPC_filter_out_create(&out);
  zRPC_filter_call_on_read(filter->filter, channel, msg, out);
  for (unsigned int i = 0; i < zRPC_filter_out_item_count(out); ++i) {
    if (filter->next == NULL) break;
    help_call_read_filter(filter->next, channel, zRPC_filter_out_get_item(out, i));
  }
  zRPC_filter_out_destroy(out);
}

void *_channel_on_active(zRPC_channel *channel) {
  zRPC_filter_linked_node *filter = channel->head;
  channel->is_active = 1;
  while (filter != NULL) {
    zRPC_filter_call_on_active(filter->filter, channel);
    filter = filter->next;
  }
  return NULL;
}

void *_channel_on_inactive(zRPC_channel *channel) {
  zRPC_filter_linked_node *filter = channel->head;
  channel->is_active = 0;
  while (filter != NULL) {
    zRPC_filter_call_on_inactive(filter->filter, channel);
    filter = filter->next;
  }
  zRPC_channel_destroy(channel);
  return NULL;
}

void *_channel_on_read(zRPC_channel *channel) {
  char temp[1024];
  ssize_t total_read, read, write;
  total_read = 0;
  do {
    read = MIN(zRPC_ring_buf_can_write(channel->buffer), 1024);
    if (read == 0)
      break;
    read = zRPC_socket_read(channel->fd, temp, (size_t) read);
    if (read <= 0) {
      if (channel->is_active) {
        zRPC_event event;
        event.event_type = EV_CLOSE;
        event.source = channel;
        zRPC_scheduler_outer_event(channel->scheduler, event);
      }
      return NULL;
    }
    if (read >= 0) {
      do {
        write = zRPC_ring_buf_write(channel->buffer, temp, (size_t) read);
      } while (write < read);
    }
    total_read += read;
  } while (read == 1024);
  if (total_read != 0) {
    zRPC_filter_linked_node *filter = channel->head;
    if (filter != NULL)
      help_call_read_filter(filter, channel, channel->buffer);
  }
  return NULL;
}

void *_channel_on_write(zRPC_channel *channel) {
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &channel->pending_write) {
    zRPC_channel_write_param *write_param = zRPC_list_entry(pos, zRPC_channel_write_param, list_node_pending);
    if (0 == write_param->write_remained) {
      goto gone;
    }

    ssize_t sent = zRPC_socket_write(channel->fd,
                                     zRPC_bytes_buf_addr(write_param->outgoing_buf) + write_param->outgoing_buf_len -
                                         write_param->write_remained, write_param->write_remained);
    if (sent < 0) {
      if (channel->is_active) {
        zRPC_event event;
        event.event_type = EV_CLOSE;
        event.source = channel;
        zRPC_scheduler_outer_event(channel->scheduler, event);
      }
      goto gone;
    }
    write_param->write_remained -= sent;
    if (write_param->write_remained) {
      break;
    }
    gone:
    zRPC_list_add_tail(&write_param->list_node_done, &channel->done_write);
  }
  zRPC_list_for_each(pos, &channel->done_write) {
    zRPC_channel_write_param *write_param = zRPC_list_entry(pos, zRPC_channel_write_param, list_node_done);
    zRPC_list_del(&write_param->list_node_pending);
    if (write_param->write_callback != NULL) {
      zRPC_runnable_run(write_param->write_callback);
    }
    free(write_param);
  }
  zRPC_list_init(&channel->done_write);
  return NULL;
}

static void channel_write_callback(zRPC_bytes_buf *buf) {
  SUB_REFERENCE(buf, zRPC_bytes_buf);
}

static void zRPC_channel_really_write(zRPC_channel *channel, zRPC_filter_out *out) {
  for (unsigned int i = 0; out != NULL && i < zRPC_filter_out_item_count(out); ++i) {
    zRPC_bytes_buf *buf = zRPC_filter_out_get_item(out, i);
    zRPC_runnable *write_callback = zRPC_runnable_create((void *(*)(void *)) channel_write_callback,
                                                         buf, zRPC_runnable_release_callback);
    zRPC_channel_write_param *write_param = malloc(sizeof(zRPC_channel_write_param));
    write_param->write_callback = write_callback;
    write_param->outgoing_buf = buf;
    write_param->outgoing_buf_len = write_param->write_remained = zRPC_bytes_buf_len(buf);

    if (!channel->is_writing) {
      channel->is_writing = 1;
      ssize_t sent = zRPC_socket_write(channel->fd,
                                       zRPC_bytes_buf_addr(write_param->outgoing_buf) +
                                           write_param->outgoing_buf_len -
                                           write_param->write_remained, write_param->write_remained);
      if (sent < 0) {
        if (channel->is_active) {
          zRPC_event event;
          event.event_type = EV_CLOSE;
          event.source = channel;
          zRPC_scheduler_outer_event(channel->scheduler, event);
        }
        if (write_param->write_callback != NULL) {
          zRPC_runnable_run(write_param->write_callback);
        }
        free(write_param);
      } else {
        write_param->write_remained -= sent;
        zRPC_list_add_tail(&write_param->list_node_pending, &channel->pending_write);
        zRPC_source_register_listener(&channel->source, EV_WRITE, 1, _event_listener_callback, NULL);
      }
      channel->is_writing = 0;
    } else {
      zRPC_list_add_tail(&write_param->list_node_pending, &channel->pending_write);
      zRPC_source_register_listener(&channel->source, EV_WRITE, 1, _event_listener_callback, NULL);
    }
  }
}

static void help_call_write_filter(zRPC_filter_linked_node *filter, zRPC_channel *channel, void *msg) {
  zRPC_filter_out *out;
  zRPC_filter_out_create(&out);
  zRPC_filter_call_on_write(filter->filter, channel, msg, out);
  for (unsigned int i = 0; i < zRPC_filter_out_item_count(out); ++i) {
    if (filter->prev == NULL) {
      zRPC_channel_really_write(channel, out);
      goto done;
    }
    help_call_write_filter(filter->prev, channel, zRPC_filter_out_get_item(out, i));
  }
  done:
  zRPC_filter_out_destroy(out);
}

void zRPC_channel_write(zRPC_channel *channel, void *msg) {
  zRPC_filter_linked_node *filter = channel->tail;
  if (filter == NULL) {
    return;
  }
  zRPC_mutex_lock(&channel->write_lock);
  help_call_write_filter(filter, channel, msg);
  zRPC_mutex_unlock(&channel->write_lock);
}