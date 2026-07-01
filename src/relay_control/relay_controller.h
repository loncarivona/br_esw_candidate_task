/**
 * @file relay_controller.h
 *
 * @brief Relay controller: global state machine over relay instances.
 *
 * Exposes Init / StopProcessing lifecycle plus a periodic Process step driven by
 * step, and owns an array of reusable relay instances. On any relay fault it
 * latches a global ERROR state, forces all relays open and blocks further close
 * requests until re-initialization (controller restart).
 *
 * Hardware access goes through the RelayIo_* interface (see relay_io.h), so
 * the controller contains application logic only.
 *
 */

#ifndef RELAY_CONTROL_RELAY_CONTROLLER_H
#define RELAY_CONTROL_RELAY_CONTROLLER_H

#include "relay_control/common.h"
#include "relay_control/relay_instance.h"

/**
 * Relay controller state.
 *
 * The layout is exposed in this header only so the application can hold a
 * controller with static allocation (no heap). All fields are PRIVATE by
 * convention (leading underscore): other modules must go through the
 * RelayController_* functions and never read or write members directly.
 */
typedef struct {
  RelayInstance _instances[kRelayMaxInstances];
  uint8_t _instance_count;
  RelayControllerState _state;
} RelayController;

/**
 * @brief Initialize relay instances from a supplied config table.
 *
 * The config is injected (not taken from a global) so multiple controller
 * instances can coexist with different relay sets, and tests can supply their
 * own table. RelayIo_Init() must be called by the application before this
 * function (once for the shared IO layer that serves all relay channels).
 *
 * @param [out] self - Controller to initialize.
 * @param [in] config - Array of relay configuration entries.
 * @param [in] count - Number of entries in @p config (<= kRelayMaxInstances).
 * @return true on success, false if @p self, @p config or @p count is invalid.
 */
bool RelayController_Init(RelayController *self, const RelayInstanceConfig *config,
                          uint8_t count);

/**
 * @brief Stop processing and force all relays open.
 *
 * @param [in,out] self - Controller whose relays are opened and processing
 *                        halted.
 * @return true if @p self is valid, false otherwise.
 */
bool RelayController_StopProcessing(RelayController *self);

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
 * @brief Get the controller-level state (NORMAL or ERROR).
 *
 * @param [in] self - Controller to query.
 * @return Current controller state, or ERROR if @p self is NULL.
 */
RelayControllerState RelayController_GetState(const RelayController *self);

/**
 * @brief Get the last sampled feedback contact state of a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @return Contact state (open/closed), or OPEN if @p self is NULL or
 *         @p relay_id is unknown.
 */
RelayContactState RelayController_GetContactState(const RelayController *self,
                                                  uint8_t relay_id);

/**
 * @brief Get the latched fault classification of a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @return Fault state (none/welded/constantly open), or NONE if @p self is
 *         NULL or @p relay_id is unknown.
 */
RelayFault RelayController_GetFault(const RelayController *self, uint8_t relay_id);

/**
 * @brief Get the currently stored open/close request for a relay.
 *
 * @param [in] self - Controller that owns the relay.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @return Stored request, or OPEN if @p self is NULL or @p relay_id is unknown.
 */
RelayCommand RelayController_GetRequest(const RelayController *self,
                                        uint8_t relay_id);

/**
 * @brief Inject an open/close request for a relay (simulated signal source).
 *
 * @param [in,out] self - Controller that owns the relay.
 * @param [in] relay_id - Logical relay identifier from the config table.
 * @param [in] request - Requested open/close command.
 */
void RelayController_SetRequest(RelayController *self, uint8_t relay_id,
                                RelayCommand request);

#endif /* RELAY_CONTROL_RELAY_CONTROLLER_H */
