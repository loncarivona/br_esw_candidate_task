/**
 * @file relay_io.h
 *
 * @brief Hardware-abstraction (HAL) interface for relay digital IO.
 *
 * Declares the operations the application needs (drive a digital power output,
 * read a digital input). This is the boundary between application logic and
 * hardware: the application only ever calls these functions. Exactly one
 * implementation is linked into the build:
 *   - relay_io_hw.c  : real MCU GPIO (target)
 *   - relay_io_sim.c : simulated plant (host demo / tests)
 *
 * This keeps a clean app/HW separation in the classic embedded-C style, without
 * runtime polymorphism.
 *
 */

#ifndef RELAY_IO_RELAY_IO_H
#define RELAY_IO_RELAY_IO_H

#include <stdbool.h>
#include <stdint.h>

/** Digital power output (DPO) drive level. */
typedef enum {
  kRelayDpoLow = 0,
  kRelayDpoHigh,
} RelayDpoLevel;

/** Digital input (DI) sampled level. */
typedef enum {
  kRelayDiLow = 0,
  kRelayDiHigh,
} RelayDiLevel;

/** @brief Initialize the digital IO (configure DPO outputs / DI inputs). */
bool RelayIo_Init(void);

/** @brief Drive the DPO of @p channel to @p level. */
void RelayIo_SetDpo(uint8_t channel, RelayDpoLevel level);

/** @brief Read the DI level of @p channel. */
RelayDiLevel RelayIo_GetDi(uint8_t channel);

#endif /* RELAY_IO_RELAY_IO_H */
