#include "outputs.h"

#include <Preferences.h>

#include "config.h"

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

namespace {

Preferences prefs;
bool switches[SWITCH_COUNT] = {false};
uint16_t dimmerDuty = 0;

// Writing every change straight to flash would wear it out under a slider
// drag, so changes are batched and flushed at most once a second.
bool dirty = false;
uint32_t dirtySince = 0;
const uint32_t kFlushDelayMs = 1000;

void applySwitch(uint8_t index) {
  const bool on = switches[index];
  digitalWrite(SWITCH_PINS[index], SWITCH_ACTIVE_LOW ? !on : on);
}

void applyDimmer() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(DIMMER_PIN, dimmerDuty);
#else
  ledcWrite(DIMMER_LEDC_CHANNEL, dimmerDuty);
#endif
}

void markDirty() {
  dirty = true;
  dirtySince = millis();
}

void flushIfDue() {
  if (!dirty || millis() - dirtySince < kFlushDelayMs) {
    return;
  }
  uint8_t bits = 0;
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    if (switches[i]) {
      bits |= (1 << i);
    }
  }
  prefs.putUChar("switches", bits);
  prefs.putUShort("dimmer", dimmerDuty);
  dirty = false;
}

} // namespace

namespace outputs {

void begin() {
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    pinMode(SWITCH_PINS[i], OUTPUT);
  }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(DIMMER_PIN, DIMMER_FREQ_HZ, DIMMER_RESOLUTION_BITS);
#else
  ledcSetup(DIMMER_LEDC_CHANNEL, DIMMER_FREQ_HZ, DIMMER_RESOLUTION_BITS);
  ledcAttachPin(DIMMER_PIN, DIMMER_LEDC_CHANNEL);
#endif

  prefs.begin("remote", false);
  if (RESTORE_STATE_ON_BOOT) {
    const uint8_t bits = prefs.getUChar("switches", 0);
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
      switches[i] = bits & (1 << i);
    }
    dimmerDuty = prefs.getUShort("dimmer", 0);
    if (dimmerDuty > DIMMER_MAX) {
      dimmerDuty = DIMMER_MAX;
    }
  }

  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    applySwitch(i);
  }
  applyDimmer();
}

bool switchState(uint8_t index) {
  return index < SWITCH_COUNT && switches[index];
}

void setSwitch(uint8_t index, bool on) {
  if (index >= SWITCH_COUNT || switches[index] == on) {
    return;
  }
  switches[index] = on;
  applySwitch(index);
  markDirty();
}

void toggleSwitch(uint8_t index) {
  if (index < SWITCH_COUNT) {
    setSwitch(index, !switches[index]);
  }
}

void setAllSwitches(bool on) {
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    setSwitch(i, on);
  }
}

uint16_t dimmer() { return dimmerDuty; }

void setDimmer(uint16_t duty) {
  if (duty > DIMMER_MAX) {
    duty = DIMMER_MAX;
  }
  if (duty == dimmerDuty) {
    return;
  }
  dimmerDuty = duty;
  applyDimmer();
  markDirty();
}

const char *switchName(uint8_t index) {
  return index < SWITCH_COUNT ? SWITCH_NAMES[index] : "";
}

void loop() { flushIfDue(); }

} // namespace outputs
