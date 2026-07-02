/**
 * @file relay_controller.c
 *
 * @brief See relay_controller.h.
 *
 */

#include "relay_control/relay_controller.h"
#include "relay_control/relay_log.h"

#include <stddef.h>

/**
 * @brief Finds the relay instance that matches the provided relay_id.
 *
 * @param [in] self - Controller whose instance array is searched.
 * @param [in] relay_id - Relay identifier from the config table.
 * @return Pointer to the matching instance, or NULL if not found
 */
static const RelayInstance *FindInstance(const RelayController *self,
                                         uint8_t relay_id) {
  if (self == NULL) {
    return NULL;
  }

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    if (RelayInstance_GetRelayId(&self->_instances[i]) == relay_id) {
      return &self->_instances[i];
    }
  }

  return NULL;
}

/**
 * @brief Reject config tables with duplicate relay_id or IO channel assignments.
 *
 * @param [in] config - Relay configuration table to validate.
 * @param [in] count - Number of entries in the config.
 * @return true if every relay_id, dpo_channel and di_channel is unique.
 */
static bool ValidateConfig(const RelayInstanceConfig *config, uint8_t count) {
  if (config == NULL || count == 0) {
    return false;
  }

  for (uint8_t i = 0; i < count; ++i) {
    for (uint8_t j = (uint8_t)(i + 1); j < count; ++j) {
      if (config[i].relay_id == config[j].relay_id) {
        LOG_ERR("Duplicate relay_id %u in configuration\n",
                      (unsigned)config[i].relay_id);
        return false;
      }

      if (config[i].dpo_channel == config[j].dpo_channel) {
        LOG_ERR("Duplicate dpo_channel %u in configuration\n",
                      (unsigned)config[i].dpo_channel);
        return false;
      }

      if (config[i].di_channel == config[j].di_channel) {
        LOG_ERR("Duplicate di_channel %u in configuration\n",
                      (unsigned)config[i].di_channel);
        return false;
      }
    }
  }

  return true;
}

/**
 * @brief Run one processing step for every relay instance.
 *
 * @param [in,out] self - Controller whose relays are processed.
 * @param [in] allow_close - Passed through to each RelayInstance_Process().
 */
static void ProcessAllRelays(RelayController *self, bool allow_close) {
  if (self == NULL) {
    return;
  }

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    RelayInstance_Process(&self->_instances[i], allow_close);
  }
}

/**
 * @brief Command all relays open and run one processing step with close blocked.
 *
 * Error-state entry action: drive every relay OPEN immediately.
 *
 * @param [in,out] self - Controller whose relays are forced open.
 */
static void ForceOpenAll(RelayController *self) {
  if (self == NULL) {
    return;
  }

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    RelayInstance_SetRequest(&self->_instances[i], kRelayCommandOpen);
    RelayInstance_Process(&self->_instances[i], false);
  }
}

/**
 * @brief Return whether any relay instance has a latched fault.
 *
 * @param [in] self - Controller whose instances are checked.
 * @return true if at least one instance reports a fault, false otherwise.
 */
static bool AnyFaultDetected(const RelayController *self) {
  if (self == NULL) {
    return false;
  }

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    if (RelayInstance_GetFault(&self->_instances[i]) != kRelayFaultNone) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Notify the application about each latched fault (On -> Error entry).
 *
 * @param [in] self - Controller entering the error state.
 */
static void NotifyFaults(RelayController *self) {
  if (self == NULL || self->_fault_notifier == NULL) {
    return;
  }

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    const RelayFault fault = RelayInstance_GetFault(&self->_instances[i]);
    if (fault != kRelayFaultNone) {
      const uint8_t relay_id = RelayInstance_GetRelayId(&self->_instances[i]);
      self->_fault_notifier(relay_id, fault, self->_fault_notifier_context);
    }
  }
}

/**
 * @brief On -> Error transition: latch error state and force all relays open.
 *
 * @param [in,out] self - Controller entering the error state.
 */
static void EnterErrorState(RelayController *self) {
  if (self == NULL) {
    return;
  }

  self->_state = kRelayControllerStateError;
  NotifyFaults(self);
  ForceOpenAll(self);
}

/**
 * @brief On-state periodic step: supervise relays and detect faults.
 *
 * @param [in,out] self - Controller in the On state.
 */
static void ProcessOnState(RelayController *self) {
  ProcessAllRelays(self, true);

  if (AnyFaultDetected(self)) {
    EnterErrorState(self);
  }
}

/**
 * @brief Error-state periodic step: keep relays open and block close requests.
 *
 * @param [in,out] self - Controller in the Error state.
 */
static void ProcessErrorState(RelayController *self) {
  ProcessAllRelays(self, false);
}

bool RelayController_Init(RelayController *self, const RelayInstanceConfig *config,
                          uint8_t count) {
  if (self == NULL) {
    LOG_ERR("Invalid relay controller\n");
    return false;
  }

  if (config == NULL || count == 0) {
    LOG_ERR("Invalid relay configuration\n");
    return false;
  }

  if (count > kRelayMaxInstances) {
    LOG_ERR("Configured relays (%u) exceed max (%u)\n", (unsigned)count,
                  (unsigned)kRelayMaxInstances);
    return false;
  }

  if (!ValidateConfig(config, count)) {
    return false;
  }

  self->_instance_count = count;
  self->_state = kRelayControllerStateOn;
  self->_fault_notifier = NULL;
  self->_fault_notifier_context = NULL;

  for (uint8_t i = 0; i < self->_instance_count; ++i) {
    RelayInstance_Init(&self->_instances[i], &config[i]);
  }

  LOG_INF("Relay controller initialized with %u relay(s)\n",
                (unsigned)count);
  return true;
}

void RelayController_Process(RelayController *self) {
  if (self == NULL) {
    return;
  }

  switch (self->_state) {
    case kRelayControllerStateOn:
      ProcessOnState(self);
      break;

    case kRelayControllerStateError:
      ProcessErrorState(self);
      break;

    default:
      break;
  }
}

RelayControllerState RelayController_GetState(const RelayController *self) {
  if (self == NULL) {
    return kRelayControllerStateError;
  }

  return self->_state;
}

RelayContactState RelayController_GetContactState(const RelayController *self,
                                                  uint8_t relay_id) {
  if (self == NULL) {
    return kRelayContactOpen;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  if (instance == NULL) {
    return kRelayContactOpen;
  }

  return RelayInstance_GetContactState(instance);
}

RelayFault RelayController_GetFault(const RelayController *self,
                                    uint8_t relay_id) {
  if (self == NULL) {
    return kRelayFaultNone;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  if (instance == NULL) {
    return kRelayFaultNone;
  }

  return RelayInstance_GetFault(instance);
}

RelayCommand RelayController_GetRequest(const RelayController *self,
                                        uint8_t relay_id) {
  if (self == NULL) {
    return kRelayCommandOpen;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  if (instance == NULL) {
    return kRelayCommandOpen;
  }

  return RelayInstance_GetRequest(instance);
}

void RelayController_SetRequest(RelayController *self, uint8_t relay_id,
                                RelayCommand request) {
  if (self == NULL) {
    return;
  }

  RelayInstance *instance = (RelayInstance *)FindInstance(self, relay_id);
  if (instance == NULL) {
    LOG_WRN("SetRequest for unknown relay_id %u ignored\n",
                  (unsigned)relay_id);
    return;
  }

  RelayInstance_SetRequest(instance, request);
}

void RelayController_SetFaultNotifier(RelayController *self,
                                      RelayFaultNotifier notifier,
                                      void *context) {
  if (self == NULL) {
    return;
  }

  self->_fault_notifier = notifier;
  self->_fault_notifier_context = context;
}
