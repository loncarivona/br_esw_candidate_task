/**
 * @file relay_instance.c
 *
 * @brief See relay_instance.h.
 *
 */

#include "relay_control/relay_instance.h"

#include <stddef.h>

/**
 * @brief Map a logical command to the DPO drive level for the relay type.
 *
 * @param [in] type - Relay type (NO or NC).
 * @param [in] command - Requested open/close command.
 * @return DPO level that realizes @p command for the given @p type.
 */
static RelayDpoLevel Relay_CommandToDpo(RelayType type, RelayCommand command) {
  if (type == kRelayTypeNormallyOpen) {
    return (command == kRelayCommandClose) ? kRelayDpoHigh : kRelayDpoLow;
  }

  return (command == kRelayCommandClose) ? kRelayDpoLow : kRelayDpoHigh;
}

/**
 * @brief Contact state expected to read back once the command has settled.
 *
 * @param [in] command - Applied open/close command.
 * @return Expected contact state (independent of relay type).
 */
static RelayContactState Relay_CommandToExpectedContact(RelayCommand command) {
  return (command == kRelayCommandClose) ? kRelayContactClosed
                                         : kRelayContactOpen;
}

/**
 * @brief Convert a sampled DI level into a contact state.
 *
 * @param [in] di_level - Sampled digital input level.
 * @return Contact state (DI high = closed, DI low = open).
 */
static RelayContactState Relay_DiToContact(RelayDiLevel di_level) {
  return (di_level == kRelayDiHigh) ? kRelayContactClosed : kRelayContactOpen;
}

/**
 * @brief Drive @p command onto the DPO and restart the feedback settle window.
 *
 * @param [in,out] self - Instance whose DPO is driven and state updated.
 * @param [in] command - Command to apply.
 */
static void ApplyCommand(RelayInstance *self, RelayCommand command) {
  const RelayDpoLevel dpo = Relay_CommandToDpo(self->_config->type, command);

  RelayIo_SetDpo(self->_config->dpo_channel, dpo);
  self->_applied_command = command;
  self->_mismatch_cycles = 0U;
  self->_match_run = 0U;
}

/**
 * @brief Latch a fault when feedback disagrees with the applied command past
 *        the settle window.
 *
 * @param [in,out] self - Instance whose feedback is validated and fault latched.
 */
static void DetectFault(RelayInstance *self) {
  /* A fault is latched until controller restart (re-init). */
  if (self->_fault != kRelayFaultNone) {
    return;
  }

  const RelayContactState expected =
      Relay_CommandToExpectedContact(self->_applied_command);

  /* Continuous check: feedback must always agree with the applied command. */
  if (self->_feedback_contact == expected) {
    if (self->_match_run < UINT16_MAX) {
      ++self->_match_run;
    }
    if (self->_match_run >= kRelayFeedbackClearCycles) {
      self->_mismatch_cycles = 0U;
    }
    return;
  }

  self->_match_run = 0U;

  if (self->_mismatch_cycles < UINT16_MAX) {
    ++self->_mismatch_cycles;
  }

  if (self->_mismatch_cycles < kRelayFeedbackSettleCycles) {
    return;
  }

  /* Mismatch persisted past the 30 ms settle window -> classify the fault.
   * Reporting is left to the controller so this component stays IO-free. */
  self->_fault = (self->_applied_command == kRelayCommandOpen)
                     ? kRelayFaultWelded
                     : kRelayFaultConstantlyOpen;
}

void RelayInstance_Init(RelayInstance *self, const RelayInstanceConfig *config) {
  if (self == NULL || config == NULL) {
    return;
  }

  self->_config = config;
  self->_request = kRelayCommandOpen;
  self->_feedback_contact = kRelayContactOpen;
  self->_fault = kRelayFaultNone;

  /* On init/startup all relays are driven to the safe OPEN state. */
  ApplyCommand(self, kRelayCommandOpen);
}

void RelayInstance_SetRequest(RelayInstance *self, RelayCommand request) {
  if (self == NULL) {
    return;
  }

  self->_request = request;
}

RelayCommand RelayInstance_GetRequest(const RelayInstance *self) {
  return (self != NULL) ? self->_request : kRelayCommandOpen;
}

void RelayInstance_Process(RelayInstance *self, bool allow_close) {
  if (self == NULL) {
    return;
  }

  RelayCommand effective_request = self->_request;

  if (!allow_close && effective_request == kRelayCommandClose) {
    effective_request = kRelayCommandOpen;
  }

  if (effective_request != self->_applied_command) {
    ApplyCommand(self, effective_request);
  }

  const RelayDiLevel feedback_level = RelayIo_GetDi(self->_config->di_channel);
  self->_feedback_contact = Relay_DiToContact(feedback_level);

  DetectFault(self);
}

RelayContactState RelayInstance_GetContactState(const RelayInstance *self) {
  return (self != NULL) ? self->_feedback_contact : kRelayContactOpen;
}

RelayFault RelayInstance_GetFault(const RelayInstance *self) {
  return (self != NULL) ? self->_fault : kRelayFaultNone;
}

const char *RelayInstance_GetName(const RelayInstance *self) {
  if (self == NULL || self->_config == NULL) {
    return "";
  }

  return self->_config->name;
}

uint8_t RelayInstance_GetRelayId(const RelayInstance *self) {
  if (self == NULL || self->_config == NULL) {
    return 0xFFU;
  }

  return self->_config->relay_id;
}
