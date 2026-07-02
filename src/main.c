/**
 * @file main.c
 *
 * @brief Host-side demo for the relay controller.
 *
 * Runs fault scenarios from CSV scripts in scenarios/ (see scenario_csv.h).
 * Each tick the simulated plant is advanced first, then the controller task is
 * run - mirroring how a periodic RTOS task executes on the target.
 *
 */

#include "relay_io/relay_io.h"
#include "relay_io/relay_io_sim.h"
#include "relay_control/common.h"
#include "relay_control/relay_controller.h"
#include "relay_control/relay_log.h"
#include "scenario/scenario_csv.h"
#include "scheduler/scheduler.h"

static const RelayInstanceConfig kRelayConfig[] = {
    {.relay_id = 0U, .type = kRelayTypeNormallyOpen, .dpo_channel = 0U,
     .di_channel = 0U, .name = "Relay_NO_1"},
    {.relay_id = 1U, .type = kRelayTypeNormallyClosed, .dpo_channel = 1U,
     .di_channel = 1U, .name = "Relay_NC_1"},
};
static const uint8_t kRelayCount =
    (uint8_t)(sizeof(kRelayConfig) / sizeof(kRelayConfig[0]));

#if defined(RELAY_LOG_ENABLED)

static const char *ContactStr(RelayContactState s) {
  return (s == kRelayContactClosed) ? "CLOSED" : "OPEN";
}

static const char *FaultStr(RelayFault f) {
  if (f == kRelayFaultWelded) return "WELDED";
  if (f == kRelayFaultConstantlyOpen) return "CONSTANTLY_OPEN";
  return "NONE";
}

static void PrintStatus(const char *scenario, uint32_t tick,
                        const RelayController *controller) {
  RELAY_LOG_RAW("%s tick=%2lu | ctrl=%s", scenario, (unsigned long)tick,
                (RelayController_GetState(controller) == kRelayControllerStateError)
                    ? "ERROR"
                    : "NORMAL");

  for (uint8_t i = 0U; i < kRelayCount; ++i) {
    uint8_t id = kRelayConfig[i].relay_id;
    RELAY_LOG_RAW(" | %s fb=%s fault=%s", kRelayConfig[i].name,
                  ContactStr(RelayController_GetContactState(controller, id)),
                  FaultStr(RelayController_GetFault(controller, id)));
  }

  RELAY_LOG_RAW("%s", "\n");
}

#endif /* RELAY_LOG_ENABLED */

static void ClearSimFaultInjection(void) {
  for (uint8_t i = 0U; i < kRelayCount; ++i) {
    const uint8_t channel = kRelayConfig[i].dpo_channel;
    RelayIoSim_SetStuckClosed(channel, false);
    RelayIoSim_SetStuckOpen(channel, false);
  }
}

static void ConfigureSimChannels(void) {
  for (uint8_t i = 0U; i < kRelayCount; ++i) {
    RelayIoSim_ConfigureChannel(kRelayConfig[i].dpo_channel,
                                kRelayConfig[i].type);
  }
}

static bool RunScenarioScript(const char *scenario_dir, const char *filename,
                              RelayController *controller,
                              Scheduler *scheduler) {
  ScenarioScript script;

  if (!ScenarioCsv_LoadFromDir(scenario_dir, filename, &script)) {
    RELAY_LOG_ERR("Failed to load scenario CSV: %s/%s\n", scenario_dir,
                  filename);
    return false;
  }

#if defined(RELAY_LOG_ENABLED)
  RELAY_LOG_RAW("\n--- %s ---\n", script.label);
#endif

  for (uint32_t tick = 0U; tick < script.tick_count; ++tick) {
    ScenarioCsv_ApplyTick(&script, tick, kRelayConfig, kRelayCount, controller);
    RelayIoSim_Update();
    Scheduler_RunTask(scheduler);
#if defined(RELAY_LOG_ENABLED)
    PrintStatus(script.label, tick, controller);
#endif
  }

  return true;
}

int main(int argc, char **argv) {
  RelayController controller;
  Scheduler scheduler;
  const char *scenario_dir = "scenarios";

  if (argc > 1) {
    scenario_dir = argv[1];
  }

  if (!RelayIo_Init()) {
    RELAY_LOG_ERR("%s", "Failed to initialize relay IO\n");
    return 1;
  }

  ConfigureSimChannels();

  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }

  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  if (!RunScenarioScript(scenario_dir, "welded.csv", &controller, &scheduler)) {
    return 1;
  }

  ClearSimFaultInjection();
  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }
  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  if (!RunScenarioScript(scenario_dir, "constantly_open.csv", &controller,
                         &scheduler)) {
    return 1;
  }

  return 0;
}
