// Switch and dimmer state, plus persistence across reboots.
#pragma once

#include <Arduino.h>

namespace outputs {

// Configures the GPIOs and, when RESTORE_STATE_ON_BOOT is set, reapplies the
// state saved before the last reset.
void begin();

// Flushes pending state to flash. Call from the main loop.
void loop();

bool switchState(uint8_t index);
void setSwitch(uint8_t index, bool on);
void toggleSwitch(uint8_t index);
void setAllSwitches(bool on);

uint16_t dimmer();
void setDimmer(uint16_t duty);

const char *switchName(uint8_t index);

} // namespace outputs
