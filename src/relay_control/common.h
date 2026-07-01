/**
 * @file common.h
 *
 * @brief Shared constants for the relay controller
 *
 */

#ifndef RELAY_CONTROL_COMMON_H
#define RELAY_CONTROL_COMMON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  kRelayTypeNormallyOpen = 0,
  kRelayTypeNormallyClosed,
} RelayType;

typedef enum {
  kRelayCommandOpen = 0,
  kRelayCommandClose,
} RelayCommand;

typedef enum {
  kRelayContactOpen = 0,
  kRelayContactClosed,
} RelayContactState;

typedef enum {
  kRelayFaultNone = 0,
  kRelayFaultWelded,
  kRelayFaultConstantlyOpen,
} RelayFault;

typedef enum {
  kRelayControllerStateNormal = 0,
  kRelayControllerStateError,
} RelayControllerState;

enum {
  kRelayTaskPeriodMs = 5,
  kRelayDpoSettleMs = 1,
  kRelayFeedbackSettleCycles = 6, /* 30 ms fault window at 5 ms task period */
  /* Matching feedback samples needed before clearing a mismatch count.
   * One sample is not enough: the contact can bounce during transitions. */
  kRelayFeedbackClearCycles = 2,
  kRelayMaxInstances = 10,
};

typedef struct {
  uint8_t relay_id;
  RelayType type;
  uint8_t dpo_channel;
  uint8_t di_channel;
  const char *name;
} RelayInstanceConfig;

#endif /* RELAY_CONTROL_COMMON_H */
