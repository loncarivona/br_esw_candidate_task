/**
 * @file main.c
 *
 * @brief Host-side demo for the relay controller.
 *
 * Runs two fault scenarios back-to-back (with controller re-init between them):
 *   1. WELDED on relay 0 (NO)
 *   2. CONSTANTLY_OPEN on relay 1 (NC)
 *
 * Each tick the simulated plant is advanced first, then the controller task is
 * run - mirroring how a periodic RTOS task executes on the target.
 *
 */

#include <stdio.h>

#include "relay_io/relay_io.h"
#include "relay_io/relay_io_sim.h"
#include "relay_control/common.h"
#include "relay_control/relay_controller.h"
#include "scheduler/scheduler.h"

#define TICKS_PER_SCENARIO 40U

static const RelayInstanceConfig kRelayConfig[] = {
    {.relay_id = 0U, .type = kRelayTypeNormallyOpen, .dpo_channel = 0U,
     .di_channel = 0U, .name = "Relay_NO_1"},
    {.relay_id = 1U, .type = kRelayTypeNormallyClosed, .dpo_channel = 1U,
     .di_channel = 1U, .name = "Relay_NC_1"},
};
static const uint8_t kRelayCount =
    (uint8_t)(sizeof(kRelayConfig) / sizeof(kRelayConfig[0]));

typedef enum {
  kDemoScenarioWelded = 0,
  kDemoScenarioConstantlyOpen,
} DemoScenario;

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
  printf("%s tick=%2lu | ctrl=%s", scenario, (unsigned long)tick,
         (RelayController_GetState(controller) == kRelayControllerStateError)
             ? "ERROR"
             : "NORMAL");

  for (uint8_t i = 0U; i < kRelayCount; ++i) {
    uint8_t id = kRelayConfig[i].relay_id;
    printf(" | %s fb=%s fault=%s", kRelayConfig[i].name,
           ContactStr(RelayController_GetContactState(controller, id)),
           FaultStr(RelayController_GetFault(controller, id)));
  }

  printf("\n");
}

static void ClearSimFaultInjection(void) {
  RelayIoSim_SetStuckClosed(0U, false);
  RelayIoSim_SetStuckOpen(0U, false);
  RelayIoSim_SetStuckClosed(1U, false);
  RelayIoSim_SetStuckOpen(1U, false);
}

static void ApplyScenarioEvent(DemoScenario scenario, uint32_t tick,
                               RelayController *controller) {
  if (tick == 5U) {
    RelayController_SetRequest(controller, 0U, kRelayCommandClose);
    RelayController_SetRequest(controller, 1U, kRelayCommandClose);
    return;
  }

  if (tick != 25U) {
    return;
  }

  if (scenario == kDemoScenarioWelded) {
    RelayIoSim_SetStuckClosed(0U, true);
    RelayController_SetRequest(controller, 0U, kRelayCommandOpen);
  } else {
    RelayIoSim_SetStuckOpen(1U, true);
    RelayController_SetRequest(controller, 1U, kRelayCommandClose);
  }
}

static void RunScenario(DemoScenario scenario, const char *label,
                        RelayController *controller, Scheduler *scheduler) {
  printf("\n--- %s ---\n", label);

  for (uint32_t tick = 0U; tick < TICKS_PER_SCENARIO; ++tick) {
    ApplyScenarioEvent(scenario, tick, controller);
    RelayIoSim_Update();
    Scheduler_RunTask(scheduler);
    PrintStatus(label, tick, controller);
  }
}

int main(void) {
  RelayController controller;
  Scheduler scheduler;

  if (!RelayIo_Init()) {
    printf("[ERR] Failed to initialize relay IO\n");
    return 1;
  }

  for (uint8_t i = 0U; i < kRelayCount; ++i) {
    RelayIoSim_ConfigureChannel(kRelayConfig[i].dpo_channel,
                                kRelayConfig[i].type);
  }

  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }

  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  RunScenario(kDemoScenarioWelded, "WELDED", &controller, &scheduler);

  ClearSimFaultInjection();
  if (!RelayController_Init(&controller, kRelayConfig, kRelayCount)) {
    return 1;
  }
  RelayIoSim_Update();
  Scheduler_Init(&scheduler, &controller);

  RunScenario(kDemoScenarioConstantlyOpen, "CONSTANTLY_OPEN", &controller,
              &scheduler);

  return 0;
}
