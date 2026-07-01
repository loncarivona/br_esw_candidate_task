/**
 * @file relay_controller.c
 *
 * @brief See relay_controller.h.
 *
 */

#include "relay_control/relay_controller.h"

#include <stddef.h>
#include <stdio.h>

/**
 * @brief Find the relay instance that matches @p relay_id.
 *
 * @param [in] self - Controller whose instance array is searched.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @return Pointer to the matching instance, or NULL if not found or @p self is
 *         NULL.
 */
static const RelayInstance *FindInstance(const RelayController *self,
                                         uint8_t relay_id) {
  if (self == NULL) {
    return NULL;
  }

  for (uint8_t i = 0U; i < self->_instance_count; ++i) {
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
 * @param [in] count - Number of entries in @p config.
 * @return true if every relay_id, dpo_channel and di_channel is unique.
 */
static bool ValidateConfig(const RelayInstanceConfig *config, uint8_t count) {
  if (config == NULL || count == 0U) {
    return false;
  }

  for (uint8_t i = 0U; i < count; ++i) {
    for (uint8_t j = (uint8_t)(i + 1U); j < count; ++j) {
      if (config[i].relay_id == config[j].relay_id) {
        printf("[ERR] Duplicate relay_id %u in configuration\n",
               (unsigned)config[i].relay_id);
        return false;
      }

      if (config[i].dpo_channel == config[j].dpo_channel) {
        printf("[ERR] Duplicate dpo_channel %u in configuration\n",
               (unsigned)config[i].dpo_channel);
        return false;
      }

      if (config[i].di_channel == config[j].di_channel) {
        printf("[ERR] Duplicate di_channel %u in configuration\n",
               (unsigned)config[i].di_channel);
        return false;
      }
    }
  }

  return true;
}

/**
 * @brief Command all relays open and run one processing step with close blocked.
 *
 * Used by the error reaction and by StopProcessing.
 *
 * @param [in,out] self - Controller whose relays are forced open.
 */
static void ForceOpenAll(RelayController *self) {
  if (self == NULL) {
    return;
  }

  for (uint8_t i = 0U; i < self->_instance_count; ++i) {
    RelayInstance_SetRequest(&self->_instances[i], kRelayCommandOpen);
    RelayInstance_Process(&self->_instances[i], false);
  }
}

/**
 * @brief Map a fault enum to a human-readable label for logging.
 *
 * @param [in] fault - Fault classification to name.
 * @return Static string label ("WELDED", "CONSTANTLY_OPEN", or "NONE").
 */
static const char *FaultName(RelayFault fault) {
  switch (fault) {
    case kRelayFaultWelded:
      return "WELDED";
    case kRelayFaultConstantlyOpen:
      return "CONSTANTLY_OPEN";
    default:
      return "NONE";
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

  for (uint8_t i = 0U; i < self->_instance_count; ++i) {
    if (RelayInstance_GetFault(&self->_instances[i]) != kRelayFaultNone) {
      return true;
    }
  }

  return false;
}

bool RelayController_Init(RelayController *self, const RelayInstanceConfig *config,
                          uint8_t count) {
  if (self == NULL) {
    printf("[ERR] Invalid relay controller (NULL)\n");
    return false;
  }

  if (config == NULL || count == 0U) {
    printf("[ERR] Invalid relay configuration\n");
    return false;
  }

  if (count > kRelayMaxInstances) {
    printf("[ERR] Configured relays (%u) exceed max (%u)\n", (unsigned)count,
           (unsigned)kRelayMaxInstances);
    return false;
  }

  if (!ValidateConfig(config, count)) {
    return false;
  }

  self->_instance_count = count;
  self->_state = kRelayControllerStateNormal;

  for (uint8_t i = 0U; i < self->_instance_count; ++i) {
    RelayInstance_Init(&self->_instances[i], &config[i]);
  }

  printf("[INF] Relay controller initialized with %u relay(s)\n",
         (unsigned)count);
  return true;
}

bool RelayController_StopProcessing(RelayController *self) {
  if (self == NULL) {
    return false;
  }

  ForceOpenAll(self);
  return true;
}

void RelayController_Process(RelayController *self) {
  if (self == NULL) {
    return;
  }

  const bool allow_close = (self->_state == kRelayControllerStateNormal);

  for (uint8_t i = 0U; i < self->_instance_count; ++i) {
    RelayInstance_Process(&self->_instances[i], allow_close);
  }

  if (self->_state == kRelayControllerStateNormal && AnyFaultDetected(self)) {
    printf("[WRN] Relay fault detected -> entering ERROR state, opening all relays\n");

    for (uint8_t i = 0U; i < self->_instance_count; ++i) {
      const RelayFault fault = RelayInstance_GetFault(&self->_instances[i]);
      if (fault != kRelayFaultNone) {
        printf("[WRN]   %s: %s\n", RelayInstance_GetName(&self->_instances[i]),
               FaultName(fault));
      }
    }

    self->_state = kRelayControllerStateError;
    ForceOpenAll(self);
  }
}

RelayControllerState RelayController_GetState(const RelayController *self) {
  return (self != NULL) ? self->_state : kRelayControllerStateError;
}

RelayContactState RelayController_GetContactState(const RelayController *self,
                                                  uint8_t relay_id) {
  if (self == NULL) {
    return kRelayContactOpen;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  return (instance != NULL) ? RelayInstance_GetContactState(instance)
                            : kRelayContactOpen;
}

RelayFault RelayController_GetFault(const RelayController *self,
                                    uint8_t relay_id) {
  if (self == NULL) {
    return kRelayFaultNone;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  return (instance != NULL) ? RelayInstance_GetFault(instance) : kRelayFaultNone;
}

RelayCommand RelayController_GetRequest(const RelayController *self,
                                        uint8_t relay_id) {
  if (self == NULL) {
    return kRelayCommandOpen;
  }

  const RelayInstance *instance = FindInstance(self, relay_id);
  return (instance != NULL) ? RelayInstance_GetRequest(instance)
                            : kRelayCommandOpen;
}

void RelayController_SetRequest(RelayController *self, uint8_t relay_id,
                                RelayCommand request) {
  if (self == NULL) {
    return;
  }

  RelayInstance *instance = (RelayInstance *)FindInstance(self, relay_id);
  if (instance == NULL) {
    printf("[WRN] SetRequest for unknown relay_id %u ignored\n",
           (unsigned)relay_id);
    return;
  }

  RelayInstance_SetRequest(instance, request);
}
