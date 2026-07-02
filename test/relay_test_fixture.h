/**
 * @file relay_test_fixture.h
 *
 * @brief Shared helpers for relay controller unit tests.
 */

#pragma once

extern "C" {
#include "relay_control/relay_controller.h"
#include "relay_io/relay_io.h"
#include "relay_io/relay_io_sim.h"
}

#include "test/common.h"

inline void RunPlantAndController(RelayController *controller, uint32_t ticks) {
  for (uint32_t i = 0; i < ticks; ++i) {
    RelayIoSim_Update();
    RelayController_Process(controller);
  }
}

inline void RunPlantAndInstance(RelayInstance *instance, bool allow_close,
                                uint32_t ticks) {
  for (uint32_t i = 0; i < ticks; ++i) {
    RelayIoSim_Update();
    RelayInstance_Process(instance, allow_close);
  }
}

class RelayControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(RelayIo_Init());
    controller_ = RelayController{};
  }

  RelayController controller_{};
};

class RelayInstanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(RelayIo_Init());
    instance_ = RelayInstance{};
  }

  RelayInstance instance_{};
};
