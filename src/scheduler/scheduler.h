/**
 * @file scheduler.h
 *
 * @brief Cyclic executive that drives the relay controller's periodic task.
 *
 * Each call to Scheduler_RunTask() represents one kRelayTaskPeriodMs task
 * period. On the target this is triggered by a hardware timer or periodic RTOS
 * task; on the host demo main() calls it in a loop (logical ticks, not wall-clock).
 *
 */

#ifndef SCHEDULER_SCHEDULER_H
#define SCHEDULER_SCHEDULER_H

#include <stdint.h>

#include "relay_control/relay_controller.h"

typedef struct {
  RelayController *_controller;
  uint32_t _tick_count;
} Scheduler;

/** @brief Bind the scheduler to the controller it will drive. */
void Scheduler_Init(Scheduler *self, RelayController *controller);

/** @brief Execute one task period: run the controller and advance the tick. */
void Scheduler_RunTask(Scheduler *self);

/** @brief Number of task periods executed so far. */
uint32_t Scheduler_GetTickCount(const Scheduler *self);

#endif /* SCHEDULER_SCHEDULER_H */
