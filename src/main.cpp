// ESP32 Wi-Fi remote control.
//
// Brings up the outputs, joins Wi-Fi (or raises its own access point) and
// serves a web UI plus a JSON API for switching channels and driving a PWM
// dimmer. See README.md for wiring and the API reference.

#include <Arduino.h>

#include "config.h"
#include "net.h"
#include "outputs.h"
#include "web_server.h"

namespace {

// Slow blink once we are on the router, fast blink while on the fallback AP,
// so the board's state is readable without a serial monitor.
void heartbeat() {
  static uint32_t last = 0;
  static bool lit = false;
  const uint32_t period = net::isStation() ? 1500 : 300;

  if (millis() - last < period) {
    return;
  }
  last = millis();
  lit = !lit;
  digitalWrite(STATUS_LED_PIN, lit);
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[boot] ESP32 remote control");

  pinMode(STATUS_LED_PIN, OUTPUT);
  outputs::begin();
  net::begin();
  web::begin();

  Serial.printf("[boot] ready at http://%s (http://%s.local)\n",
                net::ip().toString().c_str(), HOSTNAME);
}

void loop() {
  web::loop();
  net::loop();
  outputs::loop();
  heartbeat();
}
