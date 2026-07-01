/**
 * @file relay_io_sim.h
 *
 * @brief Simulation-only controls for the relay digital IO back-end.
 *
 * relay_io_sim.c implements the RelayIo_* interface by emulating the physical
 * world (contactor dynamics, feedback bounce). These extra functions let the
 * demo/test configure relay types and inject faults. They do not exist on the
 * real target, so only the simulation harness (main.c) uses them.
 *
 */

#ifndef RELAY_IO_RELAY_IO_SIM_H
#define RELAY_IO_RELAY_IO_SIM_H

#include "relay_io/relay_io.h"
#include "relay_control/common.h"

/** @brief Tell the simulation which relay type a channel drives. */
void RelayIoSim_ConfigureChannel(uint8_t channel, RelayType type);

/** @brief Inject / clear a welded (stuck closed) contact. */
void RelayIoSim_SetStuckClosed(uint8_t channel, bool stuck);

/** @brief Inject / clear a stuck-open contact. */
void RelayIoSim_SetStuckOpen(uint8_t channel, bool stuck);

/**
 * @brief Advance the simulated physical world by one task cycle.
 *
 * Represents the plant, not the controller. The demo calls this once per tick
 * before running the controller so feedback reflects the latest DPO command.
 */
void RelayIoSim_Update(void);

#endif /* RELAY_IO_RELAY_IO_SIM_H */
