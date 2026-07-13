/**
 * @file relay_types.h
 *
 * @brief Shared types and timing constants for the relay controller.
 */

#ifndef RELAY_CONTROL_RELAY_TYPES_H
#define RELAY_CONTROL_RELAY_TYPES_H

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
  kRelayControllerStateOn = 0,
  kRelayControllerStateError,
} RelayControllerState;

enum {
  kRelayTaskPeriodMs = 5,
  kRelayDpoSettleMs = 1,
  /* Spec: feedback may take up to 30 ms to stabilise after a command change. */
  kRelayFeedbackSettleMs = 30,
  kRelayFeedbackSettleCycles =
      (kRelayFeedbackSettleMs + kRelayTaskPeriodMs - 1) / kRelayTaskPeriodMs,
  /* Mismatches counted only after the settle window; one sample confirms fault. */
  kRelayFaultConfirmCycles = 1,
  kRelayMaxInstances = 10,
};

typedef struct {
  uint8_t relay_id;
  RelayType type;
  uint8_t dpo_channel;
  uint8_t di_channel;
  const char *name;
} RelayInstanceConfig;

/**
 * Optional notification when a relay fault triggers On -> Error.
 *
 * Register with RelayController_SetFaultNotifier(). NULL disables notifications.
 * Invoked once per faulted relay from EnterErrorState() only.
 */
typedef void (*RelayFaultNotifier)(uint8_t relay_id, RelayFault fault, void *context);

#endif /* RELAY_CONTROL_RELAY_TYPES_H */
