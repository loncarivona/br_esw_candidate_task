/**
 * @file relay_io_hw.c
 *
 * @brief Real-target implementation of the relay digital IO interface.
 *
 * On the MCU this would wrap the GPIO driver for the DPO/DI pins.
 * Link this file for a target build.
 *
 */

#include "relay_io/relay_io.h"

bool RelayIo_Init(void) {
  /* TODO(target): configure DPO pins as outputs and DI pins as inputs. */
  return true;
}

void RelayIo_SetDpo(uint8_t channel, RelayDpoLevel level) {
  (void)channel;
  (void)level;
  /* TODO(target): set the DPO pin for a channel to a level. */
}

RelayDiLevel RelayIo_GetDi(uint8_t channel) {
  (void)channel;
  /* TODO(target): read and return the DI level for a channel. */
  return kRelayDiLow;
}
