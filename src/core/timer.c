//
// Created by zhsyourai on 12/27/16.
//

#include "support/lock.h"
#include "zRPC/timer.h"
#include "zRPC/scheduling.h"

static void _timer_cb(void *source, zRPC_event event, void *param) {
  zRPC_timer *timer = source;
  zRPC_timer_task *task  = param;
  if (event.event_type & EV_TIMER) {
    task->callback(task->deadline, task->param);
    zRPC_list_del(&task->node);
  }
}
static void _destroy_callback(struct zRPC_event_source *source) {
  zRPC_timer *timer = container_of(source, zRPC_timer, source);
  zRPC_timer_destroy(timer);
}

zRPC_timer *zRPC_timer_create(zRPC_scheduler *scheduler) {
  zRPC_timer *timer = malloc(sizeof(zRPC_timer));
  RTTI_INIT_PTR(zRPC_timer, &timer->source);
  zRPC_source_init(&timer->source);
  zRPC_list_init(&timer->task_list);
  timer->source.destroy_callback = _destroy_callback;
  timer->scheduler = scheduler;
  zRPC_scheduler_register_source(scheduler, &timer->source);
  return timer;
}

zRPC_timer_task *zRPC_timer_timeout(zRPC_timer *timer,
                                    zRPC_timespec timeout,
                                    zPRC_timer_callback callback,
                                    void *param) {
}

zRPC_timer_task *zRPC_timer_deadline(zRPC_timer *timer,
                                     zRPC_timespec deadline,
                                     zPRC_timer_callback callback,
                                     void *param) {
  zRPC_timer_task *task = malloc(sizeof(zRPC_timer_task));
  task->triggered = 0;
  task->timer = timer;
  task->deadline = deadline;
  task->callback = callback;
  task->param = param;
  task->listener = zRPC_source_register_listener(&timer->source, EV_TIMER, 1, _timer_cb, task);
  zRPC_list_add_tail(&task->node, &timer->task_list);
  return task;
}

void zRPC_timer_destroy(zRPC_timer *timer) {
  zRPC_list_head *pos;
  zRPC_list_for_each(pos, &timer->task_list) {
    zRPC_timer_task *task = zRPC_list_entry(pos, zRPC_timer_task, node);
    zRPC_source_unregister_listener(&timer->source, task->listener);
  }
  zRPC_scheduler_unregister_source(timer->scheduler, &timer->source);
  free(timer);
}