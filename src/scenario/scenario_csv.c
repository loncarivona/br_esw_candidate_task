/**
 * @file scenario_csv.c
 *
 * @brief See scenario_csv.h.
 */

#include "scenario/scenario_csv.h"

#include <stdio.h>
#include <string.h>

#include "relay_io/relay_io_sim.h"

static bool ParseAction(const char *word, ScenarioAction *action) {
  if (strcmp(word, "close") == 0) {
    *action = kScenarioClose;
    return true;
  }
  if (strcmp(word, "open") == 0) {
    *action = kScenarioOpen;
    return true;
  }
  if (strcmp(word, "weld") == 0) {
    *action = kScenarioWeld;
    return true;
  }
  if (strcmp(word, "stuck") == 0) {
    *action = kScenarioStuck;
    return true;
  }
  return false;
}

static uint8_t DpoForRelay(const RelayInstanceConfig *config, uint8_t count,
                           uint8_t relay_id) {
  for (uint8_t i = 0; i < count; ++i) {
    if (config[i].relay_id == relay_id) {
      return config[i].dpo_channel;
    }
  }
  return 0xFF;
}

bool ScenarioCsv_LoadFromDir(const char *dir, const char *filename,
                             ScenarioScript *out) {
  if (dir == NULL || filename == NULL || out == NULL) {
    return false;
  }

  char path[256];
  if (snprintf(path, sizeof(path), "%s/%s", dir, filename) < 0) {
    return false;
  }

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    return false;
  }

  out->tick_count = kScenarioDefaultTicks;
  out->event_count = 0;

  char line[128];
  while (fgets(line, sizeof(line), file) != NULL) {
    unsigned tick = 0;
    unsigned relay = 0;
    unsigned ticks = 0;
    char action[16];

    if (line[0] == '#' || line[0] == '\n') {
      continue;
    }

    if (sscanf(line, "ticks %u", &ticks) == 1) {
      if (ticks > 0) {
        out->tick_count = ticks;
      }
      continue;
    }

    if (sscanf(line, "%u %u %15s", &tick, &relay, action) != 3) {
      continue;
    }

    if (out->event_count >= kScenarioMaxEvents) {
      fclose(file);
      return false;
    }

    ScenarioEvent *event = &out->events[out->event_count];
    if (!ParseAction(action, &event->action)) {
      fclose(file);
      return false;
    }

    event->tick = tick;
    event->relay_id = (uint8_t)relay;
    ++out->event_count;
  }

  fclose(file);
  return true;
}

static void ApplyEvent(const ScenarioEvent *event,
                       const RelayInstanceConfig *config, uint8_t config_count,
                       RelayController *controller) {
  switch (event->action) {
    case kScenarioClose:
      RelayController_SetRequest(controller, event->relay_id, kRelayCommandClose);
      break;

    case kScenarioOpen:
      RelayController_SetRequest(controller, event->relay_id, kRelayCommandOpen);
      break;

    case kScenarioWeld: {
      const uint8_t dpo = DpoForRelay(config, config_count, event->relay_id);
      if (dpo != 0xFF) {
        RelayIoSim_SetStuckClosed(dpo, true);
      }
      break;
    }

    case kScenarioStuck: {
      const uint8_t dpo = DpoForRelay(config, config_count, event->relay_id);
      if (dpo != 0xFF) {
        RelayIoSim_SetStuckOpen(dpo, true);
      }
      break;
    }

    default:
      break;
  }
}

void ScenarioCsv_ApplyTick(const ScenarioScript *script, uint32_t tick,
                           const RelayInstanceConfig *config,
                           uint8_t config_count,
                           RelayController *controller) {
  if (script == NULL || controller == NULL) {
    return;
  }

  for (uint8_t i = 0; i < script->event_count; ++i) {
    const ScenarioEvent *event = &script->events[i];
    if (event->tick == tick) {
      ApplyEvent(event, config, config_count, controller);
    }
  }
}
