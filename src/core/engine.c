//
// Created by zhsyourai on 8/2/17.
//
#include "event_engine.h"
#include "timer_engine.h"

void zRPC_event_engine_release_result(zRPC_event_engine_result *results[], size_t nresults) {
  for (int i = 0; i < nresults; ++i) {
    free(results[i]);
  }
  free(results);
}

void zRPC_timer_engine_release_result(zRPC_timer_task **results, size_t nresults) {
//  for (int i = 0; i < nresults; ++i) {
//    free(results[i]);
//  }
  free(results);
}