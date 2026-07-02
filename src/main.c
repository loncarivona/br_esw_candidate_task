/**
 * @file main.c
 *
 * @brief Host-side demo for the relay controller.
 *
 * Runs fault scenarios from CSV scripts in simulation_scenarios/ (see scenario_csv.h).
 * Each tick the simulated plant is advanced first, then the controller task is
 * run - mirroring how a periodic RTOS task executes on the target.
 *
 */

#include "relay_io/relay_io.h"
#include "relay_io/relay_io_sim.h"
#include "relay_control/relay_types.h"
#include "relay_control/relay_controller.h"
#include "relay_control/relay_log.h"
#include "scenario/scenario_csv.h"
#include "scheduler/scheduler.h"

#include <stddef.h>

static const RelayInstanceConfig kRelayConfig[] = {
    {.relay_id = 0, .type = kRelayTypeNormallyOpen, .dpo_channel = 0,
     .di_channel = 0, .name = "Relay_NO_1"},
    {.relay_id = 1, .type = kRelayTypeNormallyClosed, .dpo_channel = 1,
     .di_channel = 1, .name = "Relay_NC_1"},
};
static const uint8_t kRelayCount =
    (uint8_t)(sizeof(kRelayConfig) / sizeof(kRelayConfig[0]));

#if defined(LOG_ENABLED)

static bool s_fault_header_logged = false;

static const char *RelayNameForId(uint8_t relay_id) {
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    if (kRelayConfig[i].relay_id == relay_id) {
      return kRelayConfig[i].name;
    }
  }
  return "unknown";
}

static const char *FaultName(RelayFault fault) {
  switch (fault) {
    case kRelayFaultWelded:
      return "WELDED";
    case kRelayFaultConstantlyOpen:
      return "CONSTANTLY_OPEN";
    default:
      return "NONE";
  }
}

static void OnRelayFault(uint8_t relay_id, RelayFault fault, void *context) {
  (void)context;

  if (!s_fault_header_logged) {
    LOG_WRN(
        "Relay fault detected -> entering ERROR state, opening all relays\n");
    s_fault_header_logged = true;
  }

  LOG_WRN("\t%s: %s\n", RelayNameForId(relay_id), FaultName(fault));
}

#endif /* LOG_ENABLED */

static void ClearSimFaultInjection(void) {
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    const uint8_t channel = kRelayConfig[i].dpo_channel;
    RelayIoSim_SetStuckClosed(channel, false);
    RelayIoSim_SetStuckOpen(channel, false);
  }
}

static void ConfigureSimChannels(void) {
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    RelayIoSim_ConfigureChannel(kRelayConfig[i].dpo_channel,
                                kRelayConfig[i].type);
  }
}

static bool RunScenarioScript(const char *scenario_dir, const char *filename,
                              RelayController *controller,
                              Scheduler *scheduler) {
  ScenarioScript script;

  if (!ScenarioCsv_LoadFromDir(scenario_dir, filename, &script)) {
    LOG_ERR("Failed to load scenario CSV: %s/%s\n", scenario_dir, filename);
    return false;
  }

  for (uint32_t tick = 0; tick < script.tick_count; ++tick) {
    ScenarioCsv_ApplyTick(&script, tick, kRelayConfig, kRelayCount, controller);
    RelayIoSim_Update();
    Scheduler_RunTask(scheduler);
  }

  return true;
}

int main(int argc, char **argv) {
  RelayController controller;
  Scheduler scheduler;
  const char *scenario_dir = "simulation_scenarios";

  if (argc > 1) {
    scenario_dir = argv[1];
  }

  if (!RelayIo_Init()) {
    LOG_ERR("Failed to initialize relay IO\n");
    return 1;
  }

  ConfigureSimChannels();

#if defined(LOG_ENABLED)
  s_fault_header_logged = false;
#endif
  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }

#if defined(LOG_ENABLED)
  RelayController_SetFaultNotifier(&controller, OnRelayFault, NULL);
#endif
  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  if (!RunScenarioScript(scenario_dir, "welded.csv", &controller, &scheduler)) {
    return 1;
  }

  ClearSimFaultInjection();
#if defined(LOG_ENABLED)
  s_fault_header_logged = false;
#endif
  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }

#if defined(LOG_ENABLED)
  RelayController_SetFaultNotifier(&controller, OnRelayFault, NULL);
#endif
  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  if (!RunScenarioScript(scenario_dir, "constantly_open.csv", &controller,
                         &scheduler)) {
    return 1;
  }

  return 0;
}
