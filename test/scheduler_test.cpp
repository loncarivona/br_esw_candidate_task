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
    .relay_id = 0U,
    .type = kRelayTypeNormallyOpen,
    .dpo_channel = 0U,
    .di_channel = 0U,
    .name = "Relay_NO",
};

}  // namespace

TEST(SchedulerTest, RunTaskIncrementsTickCount) {
  // Arrange
  ASSERT_TRUE(RelayIo_Init());

  RelayController controller{};
  ASSERT_TRUE(RelayController_Init(&controller, &kNoRelay, 1U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  Scheduler scheduler{};
  Scheduler_Init(&scheduler, &controller);
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 0U);

  // Act
  for (uint32_t i = 0U; i < 5U; ++i) {
    RelayIoSim_Update();
    Scheduler_RunTask(&scheduler);
  }

  // Assert
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 5U);
}

TEST(SchedulerTest, RunTaskDrivesControllerProcessing) {
  // Arrange
  ASSERT_TRUE(RelayIo_Init());

  RelayController controller{};
  ASSERT_TRUE(RelayController_Init(&controller, &kNoRelay, 1U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  Scheduler scheduler{};
  Scheduler_Init(&scheduler, &controller);

  RelayController_SetRequest(&controller, 0U, kRelayCommandClose);

  // Act
  for (uint32_t i = 0U; i < 10U; ++i) {
    RelayIoSim_Update();
    Scheduler_RunTask(&scheduler);
  }

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller, 0U),
            kRelayContactClosed);
  EXPECT_EQ(Scheduler_GetTickCount(&scheduler), 10U);
}
