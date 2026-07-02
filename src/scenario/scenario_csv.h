/**
 * @file scenario_csv.h
 *
 * @brief Load and run host demo scenarios from CSV scripts.
 *
 * CSV format (host demo only — uses stdio, not linked on target MCU):
 *
 *   # scenario: WELDED
 *   # ticks: 40
 *   # tick,relay,action
 *   5,0,close
 *   25,0,weld
 *
 * Supported actions: close, open, weld (stuck closed), stuck_open
 */

#ifndef SCENARIO_SCENARIO_CSV_H
#define SCENARIO_SCENARIO_CSV_H

#include <stdbool.h>
#include <stdint.h>

#include "relay_control/common.h"
#include "relay_control/relay_controller.h"

enum {
  kScenarioCsvMaxEvents = 32U,
  kScenarioCsvMaxLabelLen = 64U,
  kScenarioCsvMaxPathLen = 256U,
  kScenarioCsvDefaultTicks = 40U,
};

typedef enum {
  kScenarioActionClose = 0,
  kScenarioActionOpen,
  kScenarioActionWeld,
  kScenarioActionStuckOpen,
} ScenarioAction;

typedef struct {
  uint32_t tick;
  uint8_t relay_id;
  ScenarioAction action;
} ScenarioEvent;

typedef struct {
  char label[kScenarioCsvMaxLabelLen];
  uint32_t tick_count;
  ScenarioEvent events[kScenarioCsvMaxEvents];
  uint16_t event_count;
} ScenarioScript;

/**
 * @brief Parse a scenario CSV file into @p out.
 *
 * @param [in] path - Path to the CSV file.
 * @param [out] out - Parsed script.
 * @return true on success, false if the file cannot be read or parsed.
 */
bool ScenarioCsv_Load(const char *path, ScenarioScript *out);

/**
 * @brief Build @p dir/@p filename into @p out (host demo helper).
 *
 * @return true if the path fits in @p out_size, false otherwise.
 */
bool ScenarioCsv_BuildPath(const char *dir, const char *filename, char *out,
                           uint32_t out_size);

/**
 * @brief Load a CSV scenario from @p dir/@p filename.
 */
bool ScenarioCsv_LoadFromDir(const char *dir, const char *filename,
                           ScenarioScript *out);

/**
 * @brief Apply all script events scheduled for @p tick.
 *
 * @param [in] script - Loaded scenario.
 * @param [in] tick - Current logical tick.
 * @param [in] config - Relay configuration table (for relay_id -> DPO channel).
 * @param [in] config_count - Number of config entries.
 * @param [in,out] controller - Controller receiving open/close requests.
 */
void ScenarioCsv_ApplyTick(const ScenarioScript *script, uint32_t tick,
                           const RelayInstanceConfig *config,
                           uint8_t config_count, RelayController *controller);

#endif /* SCENARIO_SCENARIO_CSV_H */
