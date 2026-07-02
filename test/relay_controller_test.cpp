/**
 * @file relay_controller_test.cpp
 *
 * @brief Unit tests for relay_controller.c.
 */

#include "relay_test_fixture.h"

namespace {

const RelayInstanceConfig kNoRelay = {
    .relay_id = 0U,
    .type = kRelayTypeNormallyOpen,
    .dpo_channel = 0U,
    .di_channel = 0U,
    .name = "Relay_NO",
};

const RelayInstanceConfig kNcRelay = {
    .relay_id = 1U,
    .type = kRelayTypeNormallyClosed,
    .dpo_channel = 1U,
    .di_channel = 1U,
    .name = "Relay_NC",
};

const RelayInstanceConfig kTwoRelays[] = {kNoRelay, kNcRelay};

}  // namespace

TEST_F(RelayControllerTest, InitValidConfig) {
  // Act
  const bool result = RelayController_Init(&controller_, kTwoRelays, 2U);

  // Assert
  ASSERT_TRUE(result);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateNormal);
  EXPECT_EQ(RelayController_GetFault(&controller_, 0U), kRelayFaultNone);
  EXPECT_EQ(RelayController_GetFault(&controller_, 1U), kRelayFaultNone);
}

TEST_F(RelayControllerTest, InitRejectsNullController) {
  // Act
  const bool result = RelayController_Init(nullptr, kTwoRelays, 2U);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsNullConfig) {
  // Act
  const bool result = RelayController_Init(&controller_, nullptr, 1U);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsZeroCount) {
  // Act
  const bool result = RelayController_Init(&controller_, kTwoRelays, 0U);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsDuplicateRelayId) {
  // Arrange
  const RelayInstanceConfig duplicate_ids[] = {
      {.relay_id = 0U,
       .type = kRelayTypeNormallyOpen,
       .dpo_channel = 0U,
       .di_channel = 0U,
       .name = "A"},
      {.relay_id = 0U,
       .type = kRelayTypeNormallyClosed,
       .dpo_channel = 1U,
       .di_channel = 1U,
       .name = "B"},
  };

  // Act
  const bool result = RelayController_Init(&controller_, duplicate_ids, 2U);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsDuplicateDpoChannel) {
  // Arrange
  const RelayInstanceConfig duplicate_dpo[] = {
      {.relay_id = 0U,
       .type = kRelayTypeNormallyOpen,
       .dpo_channel = 0U,
       .di_channel = 0U,
       .name = "A"},
      {.relay_id = 1U,
       .type = kRelayTypeNormallyClosed,
       .dpo_channel = 0U,
       .di_channel = 1U,
       .name = "B"},
  };

  // Act
  const bool result = RelayController_Init(&controller_, duplicate_dpo, 2U);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, CloseAndOpenNormallyOpenRelay) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  // Act
  RelayController_SetRequest(&controller_, 0U, kRelayCommandClose);
  RunPlantAndController(&controller_, 10U);

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0U),
            kRelayContactClosed);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateNormal);

  // Act
  RelayController_SetRequest(&controller_, 0U, kRelayCommandOpen);
  RunPlantAndController(&controller_, 10U);

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0U),
            kRelayContactOpen);
  EXPECT_EQ(RelayController_GetFault(&controller_, 0U), kRelayFaultNone);
}

TEST_F(RelayControllerTest, WeldedFaultEntersErrorState) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  RelayController_SetRequest(&controller_, 0U, kRelayCommandClose);
  RunPlantAndController(&controller_, 10U);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);

  // Act
  RelayController_SetRequest(&controller_, 0U, kRelayCommandOpen);
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 2U);

  // Assert
  EXPECT_EQ(RelayController_GetFault(&controller_, 0U), kRelayFaultWelded);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);
  EXPECT_EQ(RelayController_GetRequest(&controller_, 0U), kRelayCommandOpen);
}

TEST_F(RelayControllerTest, ConstantlyOpenFaultEntersErrorState) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNcRelay, 1U));
  RelayIoSim_ConfigureChannel(kNcRelay.dpo_channel, kNcRelay.type);

  RelayController_SetRequest(&controller_, 1U, kRelayCommandClose);
  RunPlantAndController(&controller_, 10U);

  RelayIoSim_SetStuckOpen(kNcRelay.dpo_channel, true);

  // Act
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 2U);

  // Assert
  EXPECT_EQ(RelayController_GetFault(&controller_, 1U),
            kRelayFaultConstantlyOpen);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);
}

TEST_F(RelayControllerTest, StopProcessingForcesOpen) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  RelayController_SetRequest(&controller_, 0U, kRelayCommandClose);
  RunPlantAndController(&controller_, 10U);

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0U),
            kRelayContactClosed);

  // Act
  const bool result = RelayController_StopProcessing(&controller_);
  RunPlantAndController(&controller_, 10U);

  // Assert
  ASSERT_TRUE(result);
  EXPECT_EQ(RelayController_GetRequest(&controller_, 0U), kRelayCommandOpen);
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0U),
            kRelayContactOpen);
}

TEST_F(RelayControllerTest, SetRequestForUnknownRelayIsIgnored) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1U));

  // Act
  RelayController_SetRequest(&controller_, 99U, kRelayCommandClose);

  // Assert
  EXPECT_EQ(RelayController_GetRequest(&controller_, 0U), kRelayCommandOpen);
  EXPECT_EQ(RelayController_GetRequest(&controller_, 99U), kRelayCommandOpen);
}

TEST_F(RelayControllerTest, ErrorStateBlocksCloseRequests) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, kTwoRelays, 2U));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayIoSim_ConfigureChannel(kNcRelay.dpo_channel, kNcRelay.type);

  RelayController_SetRequest(&controller_, 0U, kRelayCommandClose);
  RelayController_SetRequest(&controller_, 1U, kRelayCommandClose);
  RunPlantAndController(&controller_, 10U);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);
  RelayController_SetRequest(&controller_, 0U, kRelayCommandOpen);
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 2U);
  ASSERT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);

  // Act
  RelayController_SetRequest(&controller_, 1U, kRelayCommandClose);
  RunPlantAndController(&controller_, 5U);

  // Assert
  EXPECT_EQ(RelayController_GetRequest(&controller_, 1U), kRelayCommandClose);
  EXPECT_EQ(RelayController_GetContactState(&controller_, 1U),
            kRelayContactOpen);
}
