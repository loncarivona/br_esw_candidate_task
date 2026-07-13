/**
 * @file relay_controller_test.cpp
 *
 * @brief Unit tests for relay_controller.c.
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

const RelayInstanceConfig kTwoRelays[] = {kNoRelay, kNcRelay};

}  // namespace

TEST_F(RelayControllerTest, InitValidConfig) {
  // Act
  const bool result = RelayController_Init(&controller_, kTwoRelays, 2);

  // Assert
  ASSERT_TRUE(result);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateOn);
  EXPECT_EQ(RelayController_GetFault(&controller_, 0), kRelayFaultNone);
  EXPECT_EQ(RelayController_GetFault(&controller_, 1), kRelayFaultNone);
}

TEST_F(RelayControllerTest, InitRejectsNullController) {
  // Act
  const bool result = RelayController_Init(nullptr, kTwoRelays, 2);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsNullConfig) {
  // Act
  const bool result = RelayController_Init(&controller_, nullptr, 1);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsZeroCount) {
  // Act
  const bool result = RelayController_Init(&controller_, kTwoRelays, 0);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsDuplicateRelayId) {
  // Arrange
  const RelayInstanceConfig duplicate_ids[] = {
      {.relay_id = 0,
       .type = kRelayTypeNormallyOpen,
       .dpo_channel = 0,
       .di_channel = 0,
       .name = "A"},
      {.relay_id = 0,
       .type = kRelayTypeNormallyClosed,
       .dpo_channel = 1,
       .di_channel = 1,
       .name = "B"},
  };

  // Act
  const bool result = RelayController_Init(&controller_, duplicate_ids, 2);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, InitRejectsDuplicateDpoChannel) {
  // Arrange
  const RelayInstanceConfig duplicate_dpo[] = {
      {.relay_id = 0,
       .type = kRelayTypeNormallyOpen,
       .dpo_channel = 0,
       .di_channel = 0,
       .name = "A"},
      {.relay_id = 1,
       .type = kRelayTypeNormallyClosed,
       .dpo_channel = 0,
       .di_channel = 1,
       .name = "B"},
  };

  // Act
  const bool result = RelayController_Init(&controller_, duplicate_dpo, 2);

  // Assert
  EXPECT_FALSE(result);
}

TEST_F(RelayControllerTest, CloseAndOpenNormallyOpenRelay) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  // Act
  RelayController_SetRequest(&controller_, 0, kRelayCommandClose);
  RunPlantAndController(&controller_, 10);

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0),
            kRelayContactClosed);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateOn);

  // Act
  RelayController_SetRequest(&controller_, 0, kRelayCommandOpen);
  RunPlantAndController(&controller_, 10);

  // Assert
  EXPECT_EQ(RelayController_GetContactState(&controller_, 0),
            kRelayContactOpen);
  EXPECT_EQ(RelayController_GetFault(&controller_, 0), kRelayFaultNone);
}

TEST_F(RelayControllerTest, WeldedFaultEntersErrorState) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);

  RelayController_SetRequest(&controller_, 0, kRelayCommandClose);
  RunPlantAndController(&controller_, 10);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);

  // Act — fault is allowed only after the 30 ms settle window elapses.
  RelayController_SetRequest(&controller_, 0, kRelayCommandOpen);
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 1);

  // Assert
  EXPECT_EQ(RelayController_GetFault(&controller_, 0), kRelayFaultWelded);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);
  EXPECT_EQ(RelayController_GetRequest(&controller_, 0), kRelayCommandOpen);
}

TEST_F(RelayControllerTest, ConstantlyOpenFaultEntersErrorState) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNcRelay, 1));
  RelayIoSim_ConfigureChannel(kNcRelay.dpo_channel, kNcRelay.type);

  RelayController_SetRequest(&controller_, 1, kRelayCommandClose);
  RunPlantAndController(&controller_, 10);

  RelayIoSim_SetStuckOpen(kNcRelay.dpo_channel, true);

  // Act — fault is allowed only after the 30 ms settle window elapses.
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 1);

  // Assert
  EXPECT_EQ(RelayController_GetFault(&controller_, 1),
            kRelayFaultConstantlyOpen);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);
}

TEST_F(RelayControllerTest, SetRequestForUnknownRelayIsIgnored) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1));

  // Act
  RelayController_SetRequest(&controller_, 99, kRelayCommandClose);

  // Assert
  EXPECT_EQ(RelayController_GetRequest(&controller_, 0), kRelayCommandOpen);
  EXPECT_EQ(RelayController_GetRequest(&controller_, 99), kRelayCommandOpen);
}

struct FaultNotifierCapture {
  int call_count = 0;
  uint8_t last_relay_id = 0xFF;
  RelayFault last_fault = kRelayFaultNone;
};

void CaptureRelayFault(uint8_t relay_id, RelayFault fault, void *context) {
  auto *capture = static_cast<FaultNotifierCapture *>(context);
  ++capture->call_count;
  capture->last_relay_id = relay_id;
  capture->last_fault = fault;
}

TEST_F(RelayControllerTest, FaultNotifierCalledOnEnterErrorState) {
  // Arrange
  FaultNotifierCapture capture{};
  ASSERT_TRUE(RelayController_Init(&controller_, &kNoRelay, 1));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayController_SetFaultNotifier(&controller_, CaptureRelayFault, &capture);

  RelayController_SetRequest(&controller_, 0, kRelayCommandClose);
  RunPlantAndController(&controller_, 10);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);

  // Act — fault is allowed only after the 30 ms settle window elapses.
  RelayController_SetRequest(&controller_, 0, kRelayCommandOpen);
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 1);

  // Assert
  EXPECT_EQ(RelayController_GetFault(&controller_, 0), kRelayFaultWelded);
  EXPECT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);
  EXPECT_EQ(capture.call_count, 1);
  EXPECT_EQ(capture.last_relay_id, 0);
  EXPECT_EQ(capture.last_fault, kRelayFaultWelded);
}

TEST_F(RelayControllerTest, ErrorStateBlocksCloseRequests) {
  // Arrange
  ASSERT_TRUE(RelayController_Init(&controller_, kTwoRelays, 2));
  RelayIoSim_ConfigureChannel(kNoRelay.dpo_channel, kNoRelay.type);
  RelayIoSim_ConfigureChannel(kNcRelay.dpo_channel, kNcRelay.type);

  RelayController_SetRequest(&controller_, 0, kRelayCommandClose);
  RelayController_SetRequest(&controller_, 1, kRelayCommandClose);
  RunPlantAndController(&controller_, 10);

  RelayIoSim_SetStuckClosed(kNoRelay.dpo_channel, true);
  RelayController_SetRequest(&controller_, 0, kRelayCommandOpen);
  RunPlantAndController(&controller_, kRelayFeedbackSettleCycles + 1);
  ASSERT_EQ(RelayController_GetState(&controller_), kRelayControllerStateError);

  // Act
  RelayController_SetRequest(&controller_, 1, kRelayCommandClose);
  RunPlantAndController(&controller_, 5);

  // Assert
  EXPECT_EQ(RelayController_GetRequest(&controller_, 1), kRelayCommandClose);
  EXPECT_EQ(RelayController_GetContactState(&controller_, 1),
            kRelayContactOpen);
}
