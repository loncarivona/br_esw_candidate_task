/**
 * @file relay_instance.h
 *
 * @brief Reusable single-relay control and supervision component
 *
 * The RelayController owns an array of relay instances. Each instance drives one DPO,
 * samples one DI, continuously compares the feedback against the applied
 * command, and latches a fault when a mismatch persists longer than the feedback
 * settle window. Hardware access goes through the RelayIo_* interface, keeping
 * application logic separate from the IO layer.
 */

#ifndef RELAY_CONTROL_RELAY_INSTANCE_H
#define RELAY_CONTROL_RELAY_INSTANCE_H

#include "relay_io/relay_io.h"
#include "relay_control/relay_types.h"

typedef struct {
  const RelayInstanceConfig *_config;
  RelayCommand _request;
  RelayCommand _applied_command;
  RelayContactState _feedback_contact;
  RelayFault _fault;
  uint16_t _mismatch_cycles;
} RelayInstance;

/**
 * @brief Initialize an instance from its configuration.
 *
 * @param [out] self - Instance to initialize.
 * @param [in] config - Calibratable configuration entry for this relay.
 */
void RelayInstance_Init(RelayInstance *self, const RelayInstanceConfig *config);

/** @brief Store the latest open/close request for this relay. */
void RelayInstance_SetRequest(RelayInstance *self, RelayCommand request);

/** @brief Get the currently stored request. */
RelayCommand RelayInstance_GetRequest(const RelayInstance *self);

/**
 * @brief Run one cyclic step: apply command, sample feedback, detect faults.
 *
 * @param [in,out] self - Instance to process.
 * @param [in] allow_close - When false, close requests are forced to open
 *                           (used by the controller error state).
 */
void RelayInstance_Process(RelayInstance *self, bool allow_close);

/** @brief Last sampled contact state (open/closed). */
RelayContactState RelayInstance_GetContactState(const RelayInstance *self);

/** @brief Latched fault classification for this relay. */
RelayFault RelayInstance_GetFault(const RelayInstance *self);

/**
 * @brief Get the configured display name for this relay.
 *
 * @param [in] self - Instance to query.
 * @return Configured name string, or an empty string if invalid
 */
const char *RelayInstance_GetName(const RelayInstance *self);

/**
 * @brief Get the configured logical relay identifier.
 *
 * @param [in] self - Instance to query.
 * @return Configured relay_id, or 0xFF if invalid or not configured.
 */
uint8_t RelayInstance_GetRelayId(const RelayInstance *self);

#endif /* RELAY_CONTROL_RELAY_INSTANCE_H */
