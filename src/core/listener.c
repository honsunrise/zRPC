//
// Created by zhsyourai on 8/1/17.
//
#include "listener.h"

static void __accepter_listener_callback(void *source, zRPC_event event, void *param) {
  if(event.event_type & EV_READ) {
    zRPC_listener *listener = source;
    for (;;) {
      zRPC_inetaddr address;
      address.len = sizeof(struct sockaddr_storage);
      int new_fd = zRPC_accept4(listener->fd, &address, 1, 1);
      if (new_fd < 0) {
        switch (errno) {
          case EINTR:continue;
          case EAGAIN:return;
          default:return;
        }
      }
      zRPC_set_socket_no_sigpipe_if(new_fd);
      listener->callback(new_fd, listener->callback_arg);
    }
  }
}

static int check_socket(void) {
  int has_so_reuse_port = 0;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s >= 0) {
    has_so_reuse_port = zRPC_set_socket_reuse_port(s, 1);
    close(s);
  }
  return has_so_reuse_port;
}

static int prepare_listen_socket(int fd, const zRPC_inetaddr *address) {
  zRPC_inetaddr inetaddr_temp;
  int err = 0;
  int so_reuse_port = check_socket();
  if (so_reuse_port) {
    err = zRPC_set_socket_reuse_port(fd, 1);
    if (err != 0)
      goto error;
  }

  err = zRPC_set_socket_nonblocking(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_cloexec(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_low_latency(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_reuse_address(fd, 1);
  if (err != 0)
    goto error;
  err = zRPC_set_socket_no_sigpipe_if(fd);
  if (err != 0)
    goto error;

  if (bind(fd, (struct sockaddr *) address->addr, (socklen_t) address->len) < 0) {
    err = -1;
    goto error;
  }

  if (listen(fd, 100) < 0) {
    err = -1;
    goto error;
  }

  inetaddr_temp.len = sizeof(struct sockaddr_storage);

  if (getsockname(fd, (struct sockaddr *) inetaddr_temp.addr,
                  (socklen_t *) &inetaddr_temp.len) < 0) {
    err = -1;
    goto error;
  }

  return 0;

  error:
  if (fd >= 0) {
    close(fd);
  }
  return err;
}

static void _destroy_callback(struct zRPC_event_source *source) {
  zRPC_listener *listener = container_of(source, zRPC_listener, source);
  zRPC_listener_destroy(listener);
}


void zRPC_listener_create(zRPC_listener **out,
                          zRPC_socket_mode *smode,
                          zRPC_inetaddr address,
                          accepter_listener_callback callback,
                          void *param,
                          zRPC_scheduler *scheduler) {
  zRPC_listener *listener = (zRPC_listener *) malloc(sizeof(zRPC_listener));
  RTTI_INIT_PTR(zRPC_listener, &listener->source);
  zRPC_source_init(&listener->source);
  listener->source.destroy_callback = _destroy_callback;
  listener->fd = 0;
  listener->address = address;
  listener->callback = callback;
  listener->callback_arg = param;
  listener->scheduler = scheduler;
  int errs[2];
  errs[0] = zRPC_create_socket(&address, SOCK_STREAM, 0, smode, &listener->fd);
  if (errs[0] == 0) {
    prepare_listen_socket(listener->fd, &address);
    listener->event_listener =
        zRPC_source_register_listener(&listener->source, EV_READ, 0, __accepter_listener_callback, NULL);
    zRPC_scheduler_register_source(scheduler, &listener->source);
    *out = listener;
  } else {
    *out = NULL;
  }
}

void zRPC_listener_destroy(zRPC_listener *listener) {
  zRPC_source_unregister_listener(&listener->source, listener->event_listener);
  zRPC_scheduler_unregister_source(listener->scheduler, &listener->source);
  close(listener->fd);
  free(listener);
}