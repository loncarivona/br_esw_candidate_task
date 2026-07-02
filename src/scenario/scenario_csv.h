/**
 * @file scenario_csv.h
 *
 * @brief Load and run host demo scenarios from simple text scripts.
 *
 *
 * Format (one directive or event per line):
 *
 *   ticks 40
 *   5 0 close
 *   25 0 weld
 *
 * Actions: close, open, weld, stuck
 */

#ifndef SCENARIO_SCENARIO_CSV_H
#define SCENARIO_SCENARIO_CSV_H

#include <stdbool.h>
#include <stdint.h>

#include "relay_control/relay_types.h"
#include "relay_control/relay_controller.h"

enum {
  kScenarioMaxEvents = 16,
  kScenarioDefaultTicks = 40,
};

typedef enum {
  kScenarioClose = 0,
  kScenarioOpen,
  kScenarioWeld,
  kScenarioStuck,
} ScenarioAction;

typedef struct {
  uint32_t tick;
  uint8_t relay_id;
  ScenarioAction action;
} ScenarioEvent;

typedef struct {
  uint32_t tick_count;
  uint8_t event_count;
  ScenarioEvent events[kScenarioMaxEvents];
} ScenarioScript;

bool ScenarioCsv_LoadFromDir(const char *dir, const char *filename,
                             ScenarioScript *out);

void ScenarioCsv_ApplyTick(const ScenarioScript *script, uint32_t tick,
                           const RelayInstanceConfig *config,
                           uint8_t config_count, RelayController *controller);

#endif /* SCENARIO_SCENARIO_CSV_H */
