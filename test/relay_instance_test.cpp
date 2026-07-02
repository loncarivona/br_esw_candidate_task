/**
 * @file relay_instance_test.cpp
 *
 * @brief Unit tests for relay_instance.c.
 */

#include "relay_test_fixture.h"

namespace {

const RelayInstanceConfig kNoRelay = {
    .relay_id = 0,
    .type = kRelayTypeNormallyOpen,
    .dpo_channel = 0,
    .di_channel = 0,
    .name = "Relay_NO",
};

const RelayInstanceConfig kNcRelay = {
    .relay_id = 1,
    .type = kRelayTypeNormallyClosed,
    .dpo_channel = 1,
    .di_channel = 1,
    .name = "Relay_NC",
};

}  // namespace

TEST_F(RelayInstanceTest, InitDrivesRelayOpen) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayInstance_Init(&instance_, &kNoRelay);

  // Act
  RunPlantAndInstance(&instance_, true, 5);

  // Assert
  EXPECT_EQ(RelayInstance_GetRequest(&instance_), kRelayCommandOpen);
  EXPECT_EQ(RelayInstance_GetContactState(&instance_), kRelayContactOpen);
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultNone);
}

TEST_F(RelayInstanceTest, CloseAndOpenNormallyOpenRelay) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayInstance_Init(&instance_, &kNoRelay);

  // Act
  RelayInstance_SetRequest(&instance_, kRelayCommandClose);
  RunPlantAndInstance(&instance_, true, 10);

  // Assert
  EXPECT_EQ(RelayInstance_GetContactState(&instance_), kRelayContactClosed);

  // Act
  RelayInstance_SetRequest(&instance_, kRelayCommandOpen);
  RunPlantAndInstance(&instance_, true, 10);

  // Assert
  EXPECT_EQ(RelayInstance_GetContactState(&instance_), kRelayContactOpen);
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultNone);
}

TEST_F(RelayInstanceTest, DetectsWeldedFault) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayInstance_Init(&instance_, &kNoRelay);

  RelayInstance_SetRequest(&instance_, kRelayCommandClose);
  RunPlantAndInstance(&instance_, true, 10);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);

  // Act
  RelayInstance_SetRequest(&instance_, kRelayCommandOpen);
  RunPlantAndInstance(&instance_, true, kRelayFeedbackSettleCycles + 2);

  // Assert
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultWelded);
}

TEST_F(RelayInstanceTest, DetectsConstantlyOpenFaultOnNormallyClosedRelay) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNcRelay.dpo_channel, kNcRelay.type);
  RelayInstance_Init(&instance_, &kNcRelay);

  RelayInstance_SetRequest(&instance_, kRelayCommandClose);
  RunPlantAndInstance(&instance_, true, 10);

  RelayIoSim_SetStuckOpen(kNcRelay.dpo_channel, true);

  // Act
  RunPlantAndInstance(&instance_, true, kRelayFeedbackSettleCycles + 2);

  // Assert
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultConstantlyOpen);
}

TEST_F(RelayInstanceTest, AllowCloseFalseForcesOpenCommand) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayInstance_Init(&instance_, &kNoRelay);

  RelayInstance_SetRequest(&instance_, kRelayCommandClose);

  // Act
  RunPlantAndInstance(&instance_, false, 10);

  // Assert
  EXPECT_EQ(RelayInstance_GetContactState(&instance_), kRelayContactOpen);
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultNone);
}

TEST_F(RelayInstanceTest, BounceDuringCloseDoesNotLatchFault) {
  // Arrange
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayInstance_Init(&instance_, &kNoRelay);

  RelayInstance_SetRequest(&instance_, kRelayCommandClose);

  // Act 
  RunPlantAndInstance(&instance_, true, 20);

  // Assert
  EXPECT_EQ(RelayInstance_GetContactState(&instance_), kRelayContactClosed);
  EXPECT_EQ(RelayInstance_GetFault(&instance_), kRelayFaultNone);
}

TEST_F(RelayInstanceTest, ExposesConfiguredMetadata) {
  // Arrange
  RelayInstance_Init(&instance_, &kNoRelay);

  // Act
  const char *name = RelayInstance_GetName(&instance_);
  const uint8_t relay_id = RelayInstance_GetRelayId(&instance_);

  // Assert
  EXPECT_STREQ(name, "Relay_NO");
  EXPECT_EQ(relay_id, 0);
}
