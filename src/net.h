// Wi-Fi bring-up: join the configured network, fall back to an access point,
// and keep retrying the network in the background.
#pragma once

#include <Arduino.h>

namespace net {

void begin();

// Handles reconnection attempts. Call from the main loop.
void loop();

bool isStation();          // true when joined to your router
String modeName();         // "sta" or "ap"
String ssid();             // network joined, or the AP we are broadcasting
IPAddress ip();            // address the web UI is reachable on
int rssi();                // signal strength in dBm, 0 in AP mode

} // namespace net
