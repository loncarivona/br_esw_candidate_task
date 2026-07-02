/**
 * @file scenario_csv.c
 *
 * @brief See scenario_csv.h.
 */

#include "scenario/scenario_csv.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "relay_io/relay_io_sim.h"

static void InitScript(ScenarioScript *out) {
  out->label[0] = '\0';
  out->tick_count = kScenarioCsvDefaultTicks;
  out->event_count = 0U;
}

static char *Trim(char *s) {
  while (*s != '\0' && isspace((unsigned char)*s)) {
    ++s;
  }

  if (*s == '\0') {
    return s;
  }

  char *end = s + strlen(s) - 1U;
  while (end > s && isspace((unsigned char)*end)) {
    *end = '\0';
    --end;
  }

  return s;
}

static bool ParseAction(const char *token, ScenarioAction *action) {
  if (strcmp(token, "close") == 0) {
    *action = kScenarioActionClose;
    return true;
  }

  if (strcmp(token, "open") == 0) {
    *action = kScenarioActionOpen;
    return true;
  }

  if (strcmp(token, "weld") == 0) {
    *action = kScenarioActionWeld;
    return true;
  }

  if (strcmp(token, "stuck_open") == 0) {
    *action = kScenarioActionStuckOpen;
    return true;
  }

  return false;
}

static bool ParseComment(const char *line, ScenarioScript *out) {
  if (strncmp(line, "scenario:", 9U) == 0) {
    const char *value = Trim((char *)(line + 9U));
    (void)snprintf(out->label, sizeof(out->label), "%s", value);
    return true;
  }

  if (strncmp(line, "ticks:", 6U) == 0) {
    const char *value = Trim((char *)(line + 6U));
    const unsigned long ticks = strtoul(value, NULL, 10);
    if (ticks > 0UL) {
      out->tick_count = (uint32_t)ticks;
    }
    return true;
  }

  return false;
}

static bool ParseEventLine(char *line, ScenarioScript *out) {
  if (out->event_count >= kScenarioCsvMaxEvents) {
    return false;
  }

  char *comma1 = strchr(line, ',');
  if (comma1 == NULL) {
    return false;
  }

  char *comma2 = strchr(comma1 + 1U, ',');
  if (comma2 == NULL) {
    return false;
  }

  *comma1 = '\0';
  *comma2 = '\0';

  char *tick_token = Trim(line);
  char *relay_token = Trim(comma1 + 1U);
  char *action_token = Trim(comma2 + 1U);

  ScenarioEvent *event = &out->events[out->event_count];
  ScenarioAction action;

  if (!ParseAction(action_token, &action)) {
    return false;
  }

  event->tick = (uint32_t)strtoul(tick_token, NULL, 10);
  event->relay_id = (uint8_t)strtoul(relay_token, NULL, 10);
  event->action = action;
  ++out->event_count;
  return true;
}

static bool FindDpoChannel(const RelayInstanceConfig *config, uint8_t config_count,
                           uint8_t relay_id, uint8_t *dpo_channel) {
  for (uint8_t i = 0U; i < config_count; ++i) {
    if (config[i].relay_id == relay_id) {
      *dpo_channel = config[i].dpo_channel;
      return true;
    }
  }

  return false;
}

bool ScenarioCsv_BuildPath(const char *dir, const char *filename, char *out,
                           uint32_t out_size) {
  if (dir == NULL || filename == NULL || out == NULL || out_size == 0U) {
    return false;
  }

  const int written = snprintf(out, (size_t)out_size, "%s/%s", dir, filename);
  return (written > 0) && ((uint32_t)written < out_size);
}

bool ScenarioCsv_LoadFromDir(const char *dir, const char *filename,
                             ScenarioScript *out) {
  char path[kScenarioCsvMaxPathLen];

  if (!ScenarioCsv_BuildPath(dir, filename, path, (uint32_t)sizeof(path))) {
    return false;
  }

  return ScenarioCsv_Load(path, out);
}

bool ScenarioCsv_Load(const char *path, ScenarioScript *out) {
  if (path == NULL || out == NULL) {
    return false;
  }

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    return false;
  }

  InitScript(out);

  char line[160];
  while (fgets(line, sizeof(line), file) != NULL) {
    char *content = Trim(line);
    if (*content == '\0') {
      continue;
    }

    if (*content == '#') {
      ++content;
      content = Trim(content);
      (void)ParseComment(content, out);
      continue;
    }

    if (!ParseEventLine(content, out)) {
      fclose(file);
      return false;
    }
  }

  fclose(file);

  if (out->label[0] == '\0') {
    (void)snprintf(out->label, sizeof(out->label), "%s", path);
  }

  return true;
}

static void ApplyEvent(const ScenarioEvent *event,
                       const RelayInstanceConfig *config, uint8_t config_count,
                       RelayController *controller) {
  uint8_t dpo_channel = 0U;

  switch (event->action) {
    case kScenarioActionClose:
      RelayController_SetRequest(controller, event->relay_id, kRelayCommandClose);
      break;

    case kScenarioActionOpen:
      RelayController_SetRequest(controller, event->relay_id, kRelayCommandOpen);
      break;

    case kScenarioActionWeld:
      if (FindDpoChannel(config, config_count, event->relay_id, &dpo_channel)) {
        RelayIoSim_SetStuckClosed(dpo_channel, true);
      }
      break;

    case kScenarioActionStuckOpen:
      if (FindDpoChannel(config, config_count, event->relay_id, &dpo_channel)) {
        RelayIoSim_SetStuckOpen(dpo_channel, true);
      }
      break;

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

  for (uint16_t i = 0U; i < script->event_count; ++i) {
    const ScenarioEvent *event = &script->events[i];
    if (event->tick == tick) {
      ApplyEvent(event, config, config_count, controller);
    }
  }
}
