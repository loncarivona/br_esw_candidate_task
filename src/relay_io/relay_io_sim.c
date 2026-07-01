/**
 * @file relay_io_sim.c
 *
 * @brief Simulated implementation of the relay digital IO interface.
 *
 * Models the physical world so the controller can run on a host:
 * - DPO HIGH energizes the coil; DPO LOW de-energizes it.
 * - DI HIGH means feedback contact closed; DI LOW means open.
 * - Physical contact follows the applied coil state after kRelayDpoSettleMs.
 * - Short feedback bouncing is modelled for a couple of cycles on transitions.
 *
 */

#include "relay_io/relay_io_sim.h"

#include <string.h>

#include "relay_control/common.h"

typedef struct {
  RelayDpoLevel commanded_dpo;
  RelayDpoLevel applied_dpo;
  RelayContactState physical_contact;
  RelayDiLevel reported_di;
  uint8_t transition_cycles;
  uint8_t bounce_cycles;
  bool stuck_closed;
  bool stuck_open;
  RelayType relay_type;
} SimChannel;

static SimChannel s_channels[kRelayMaxInstances];

static RelayContactState ContactFromDpo(RelayDpoLevel dpo, RelayType type) {
  if (type == kRelayTypeNormallyOpen) {
    return (dpo == kRelayDpoHigh) ? kRelayContactClosed : kRelayContactOpen;
  }

  return (dpo == kRelayDpoLow) ? kRelayContactClosed : kRelayContactOpen;
}

static RelayDiLevel DiFromContact(RelayContactState contact) {
  return (contact == kRelayContactClosed) ? kRelayDiHigh : kRelayDiLow;
}

static RelayContactState ResolvePhysicalContact(const SimChannel *channel) {
  if (channel->stuck_closed) {
    return kRelayContactClosed;
  }

  if (channel->stuck_open) {
    return kRelayContactOpen;
  }

  return ContactFromDpo(channel->applied_dpo, channel->relay_type);
}

bool RelayIo_Init(void) {
  memset(s_channels, 0, sizeof(s_channels));

  for (uint8_t i = 0U; i < kRelayMaxInstances; ++i) {
    s_channels[i].commanded_dpo = kRelayDpoLow;
    s_channels[i].applied_dpo = kRelayDpoLow;
    s_channels[i].physical_contact = kRelayContactOpen;
    s_channels[i].reported_di = kRelayDiLow;
    s_channels[i].relay_type = kRelayTypeNormallyOpen;
  }

  return true;
}

void RelayIo_SetDpo(uint8_t channel, RelayDpoLevel level) {
  if (channel >= kRelayMaxInstances) {
    return;
  }

  if (s_channels[channel].commanded_dpo != level) {
    s_channels[channel].commanded_dpo = level;
    s_channels[channel].transition_cycles =
        (uint8_t)((kRelayDpoSettleMs + kRelayTaskPeriodMs - 1U) /
                  kRelayTaskPeriodMs);
  }
}

RelayDiLevel RelayIo_GetDi(uint8_t channel) {
  if (channel >= kRelayMaxInstances) {
    return kRelayDiLow;
  }

  return s_channels[channel].reported_di;
}

void RelayIoSim_ConfigureChannel(uint8_t channel, RelayType type) {
  if (channel >= kRelayMaxInstances) {
    return;
  }

  s_channels[channel].relay_type = type;

  /* A de-energized NC relay rests closed. */
  if (type == kRelayTypeNormallyClosed) {
    s_channels[channel].physical_contact = kRelayContactClosed;
    s_channels[channel].reported_di = kRelayDiHigh;
  }
}

void RelayIoSim_SetStuckClosed(uint8_t channel, bool stuck) {
  if (channel < kRelayMaxInstances) {
    s_channels[channel].stuck_closed = stuck;
    if (stuck) {
      s_channels[channel].stuck_open = false;
    }
  }
}

void RelayIoSim_SetStuckOpen(uint8_t channel, bool stuck) {
  if (channel < kRelayMaxInstances) {
    s_channels[channel].stuck_open = stuck;
    if (stuck) {
      s_channels[channel].stuck_closed = false;
    }
  }
}

void RelayIoSim_Update(void) {
  for (uint8_t i = 0U; i < kRelayMaxInstances; ++i) {
    SimChannel *channel = &s_channels[i];

    if (channel->transition_cycles > 0U) {
      --channel->transition_cycles;
      if (channel->transition_cycles == 0U) {
        channel->applied_dpo = channel->commanded_dpo;
        channel->bounce_cycles = 2U;
      }
    }

    channel->physical_contact = ResolvePhysicalContact(channel);

    if (channel->bounce_cycles > 0U) {
      --channel->bounce_cycles;
      channel->reported_di =
          (channel->reported_di == kRelayDiHigh) ? kRelayDiLow : kRelayDiHigh;
    } else {
      channel->reported_di = DiFromContact(channel->physical_contact);
    }
  }
}
