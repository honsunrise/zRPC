//
// Created by zhsyourai on 7/24/17.
//
#include <unistd.h>
#include "fd_notifiable.h"
#include "rtti.h"
#include "notify.h"

static void _notify_cb(void *source, zRPC_event event, void *param) {
  if (event.event_type & EV_READ) {
    unsigned char buf[8];
    struct zRPC_notify *notify = source;
    while (read(notify->notify_fd[0], (char *) buf, sizeof(buf)) > 0);
    notify->is_notify_pending = 0;
  }
}

static void _destroy_callback(struct zRPC_event_source *source) {
  zRPC_notify *notify = container_of(source, zRPC_notify, source);
  zRPC_notify_destroy(notify);
}

void zRPC_notify_create(zRPC_notify **out) {
  zRPC_notify *notify = (zRPC_notify *) malloc(sizeof(zRPC_notify));
  RTTI_INIT_PTR(zRPC_notify, &notify->source);
  zRPC_create_notifiable_fd(notify->notify_fd);
  zRPC_source_init(&notify->source);
  notify->source.destroy_callback = _destroy_callback;
  zRPC_source_register_listener(&notify->source, EV_READ | EV_CLOSE | EV_ERROR, 0, _notify_cb, 0);
  *out = notify;
}

void zRPC_notify_destroy(zRPC_notify *notify) {
  zRPC_destroy_notifiable_fd(notify->notify_fd);
  zRPC_source_unregister_listener(&notify->source, EV_READ | EV_CLOSE | EV_ERROR, _notify_cb);
  free(notify);
}

void zRPC_notify_write(zRPC_notify *notify) {
  if (notify->is_notify_pending) {
    return;
  }
  char buf[1];
  buf[0] = (char) 0;
  notify->is_notify_pending = 1;
  write(notify->notify_fd[1], buf, 1);
}