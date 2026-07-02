/**
 * @file scheduler_test.cpp
 *
 * @brief Unit tests for scheduler.c.
 */

#include "relay_test_fixture.h"

extern "C" {
#include "scheduler/scheduler.h"
}

namespace {

const RelayInstanceConfig kNoRelay = {
    .relay_id = 0,
    .type = kRelayTypeNormallyOpen,
    .dpo_channel = 0,
    .di_channel = 0,
    .name = "Relay_NO",
};

}  // namespace

TEST(SchedulerTest, RunTaskIncrementsTickCount) {
  // Arrange
  ASSERT_TRUE(RelayIo_Init());

  RelayController controller{};
  ASSERT_TRUE(RelayController_Init(&controller, &kNoRelay, 1));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  Scheduler scheduler{};
  Scheduler_Init(&scheduler, &controller);
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 0);

  // Act
  for (uint32_t i = 0; i < 5; ++i) {
    RelayIoSim_Update();
    Scheduler_RunTask(&scheduler);
  }

  // Assert
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 5);
}

TEST(SchedulerTest, RunTaskDrivesControllerProcessing) {
  // Arrange
  ASSERT_TRUE(RelayIo_Init());

  RelayController controller{};
  ASSERT_TRUE(RelayController_Init(&controller, &kNoRelay, 1));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  Scheduler scheduler{};
  Scheduler_Init(&scheduler, &controller);

  RelayController_SetRequest(&controller, 0, kRelayCommandClose);

  // Act
  for (uint32_t i = 0; i < 10; ++i) {
    RelayIoSim_Update();
    Scheduler_RunTask(&scheduler);
  }

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller, 0),
            kRelayContactClosed);
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 10);
}
