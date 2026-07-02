/**
 * @file relay_io.h
 *
 * @brief Hardware-abstraction (HAL) interface for relay digital IO.
 *
 * Declares functions to control and read state of GPIO (DPO and DI)
 *  Only one implementation is linked into the build:
 *   - relay_io_hw.c  : real MCU GPIO (target)
 *   - relay_io_sim.c : simulated demo
 *
 * Keeps a clean app/HW separation without runtime polymorphism
 *
 */

#ifndef RELAY_IO_RELAY_IO_H
#define RELAY_IO_RELAY_IO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  kRelayDpoLow = 0,
  kRelayDpoHigh,
} RelayDpoLevel;

typedef enum {
  kRelayDiLow = 0,
  kRelayDiHigh,
} RelayDiLevel;

/** @brief Initialize the digital IO and configure DPO as outputs and DI inputs). */
bool RelayIo_Init(void);

/** @brief Drive the DPO of a channel to requested level. */
void RelayIo_SetDpo(uint8_t channel, RelayDpoLevel level);

/** @brief Read the DI level of a channel. */
RelayDiLevel RelayIo_GetDi(uint8_t channel);

#endif /* RELAY_IO_RELAY_IO_H */
