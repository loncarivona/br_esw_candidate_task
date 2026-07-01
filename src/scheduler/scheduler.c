/**
 * @file scheduler.c
 *
 * @brief See scheduler.h.
 *
 */

#include "scheduler/scheduler.h"

void Scheduler_Init(Scheduler *self, RelayController *controller) {
  self->_controller = controller;
  self->_tick_count = 0U;
}

void Scheduler_RunTask(Scheduler *self) {
  RelayController_Process(self->_controller);
  ++self->_tick_count;
}

uint32_t Scheduler_GetTickCount(const Scheduler *self) {
  return self->_tick_count;
}
