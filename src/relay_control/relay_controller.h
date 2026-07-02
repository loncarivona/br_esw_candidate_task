/**
 * @file relay_controller.h
 *
 * @brief Relay controller: global state machine over relay instances.
 *
 * Exposes functions for the relays control and monitoring
 * On any relay fault it latches a global ERROR state, forces all relays open and blocks further close
 * requests until controller restart.
 *
 * Hardware access goes through the RelayIo_* interface (see relay_io.h), so
 * the controller contains application logic only.
 *
 */

#ifndef RELAY_CONTROL_RELAY_CONTROLLER_H
#define RELAY_CONTROL_RELAY_CONTROLLER_H

#include "relay_control/relay_types.h"
#include "relay_control/relay_instance.h"

typedef struct {
  RelayInstance _instances[kRelayMaxInstances];
  uint8_t _instance_count;
  RelayControllerState _state;
  RelayFaultNotifier _fault_notifier;
  void *_fault_notifier_context;
} RelayController;

/**
 * @brief Initialize relay instances from a supplied config table.
 *
 * The config is injected to allow for multiple controller instances
 * Note: RelayIo_Init() must be called before this function
 *
 * @param [out] self - Controller to initialize.
 * @param [in] config - Array of relay configuration entries.
 * @param [in] count - Number of entries in the config table
 * @return true on success, false if parameters are invalid.
 */
bool RelayController_Init(RelayController *self, const RelayInstanceConfig *config,
                          uint8_t count);

/**
 * @brief One periodic task step (driven by the scheduler).
 *
 * Runs every relay instance, detects faults and latches the global ERROR state
 * when any instance reports a fault.
 *
 * @param [in,out] self - Controller to process.
 */
void RelayController_Process(RelayController *self);

/**
 * @brief Get the controller-level state (On or Error).
 *
 * @param [in] self - Controller to query.
 * @return Current controller state, or ERROR
 */
RelayControllerState RelayController_GetState(const RelayController *self);

/**
 * @brief Get the last sampled feedback contact state of a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @return Contact state (open/closed), or OPEN if parameters are invalid
 */
RelayContactState RelayController_GetContactState(const RelayController *self,
                                                  uint8_t relay_id);

/**
 * @brief Get the latched fault classification of a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Relay identifier from the config table.
 * @return Fault state (none/welded/constantly open), or NONE if parameters are invalid
 */
RelayFault RelayController_GetFault(const RelayController *self, uint8_t relay_id);

/**
 * @brief Get the currently stored open/close request for a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Relay identifier from the config table.
 * @return Stored request, or OPEN if parameters are invalid.
 */
RelayCommand RelayController_GetRequest(const RelayController *self,
                                        uint8_t relay_id);

/**
 * @brief Inject an open/close request for a relay (simulated signal source).
 *
 * @param [in,out] self - Controller that owns the relay.
 * @param [in] relay_id - Relay identifier from the config table.
 * @param [in] request - Requested open/close command.
 */
void RelayController_SetRequest(RelayController *self, uint8_t relay_id,
                                RelayCommand request);

/**
 * @brief Register an optional notifier for relay faults on On -> Error entry.
 *
 * The notifier runs synchronously from EnterErrorState(), once per faulted relay.
 * Passing NULL disables notifications.
 *
 * @param [in,out] self - Controller to configure.
 * @param [in] notifier - Application handler, or NULL to disable.
 * @param [in] context - Opaque pointer passed to each notification.
 */
void RelayController_SetFaultNotifier(RelayController *self,
                                      RelayFaultNotifier notifier,
                                      void *context);

#endif /* RELAY_CONTROL_RELAY_CONTROLLER_H */
