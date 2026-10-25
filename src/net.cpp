#include "net.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "config.h"

namespace {

bool station = false;
uint32_t lastRetry = 0;
bool mdnsStarted = false;

bool haveCredentials() { return strlen(WIFI_SSID) > 0; }

void startMdns() {
  if (mdnsStarted) {
    return;
  }
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    mdnsStarted = true;
    Serial.printf("[net] mDNS ready at http://%s.local\n", HOSTNAME);
  }
}

// Blocks until connected or WIFI_CONNECT_TIMEOUT_MS elapses.
bool joinNetwork() {
  Serial.printf("[net] joining \"%s\"", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[net] join failed");
    return false;
  }
  Serial.printf("[net] connected, IP %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[net] access point \"%s\" up, IP %s\n", AP_SSID,
                WiFi.softAPIP().toString().c_str());
  if (!haveCredentials()) {
    Serial.println("[net] no credentials set - see include/secrets.h.example");
  }
}

} // namespace

namespace net {

void begin() {
  WiFi.persistent(false);
  WiFi.setHostname(HOSTNAME);

  if (haveCredentials()) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    station = joinNetwork();
  }

  if (!station) {
    startAccessPoint();
  }
  startMdns();
  lastRetry = millis();
}

void loop() {
  // In station mode the core reconnects for us; just track the flag.
  if (station) {
    station = WiFi.status() == WL_CONNECTED;
    if (station) {
      startMdns();
    }
    return;
  }

  // Sitting on the fallback AP. Periodically try the real network again so an
  // ESP32 that booted before the router came up recovers on its own.
  if (!haveCredentials() || millis() - lastRetry < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  lastRetry = millis();

  Serial.println("[net] retrying the configured network");
  WiFi.mode(WIFI_AP_STA);
  if (joinNetwork()) {
    station = true;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    startMdns();
  } else {
    WiFi.mode(WIFI_AP);
  }
}

bool isStation() { return station; }

String modeName() { return station ? "sta" : "ap"; }

String ssid() { return station ? WiFi.SSID() : String(AP_SSID); }

IPAddress ip() { return station ? WiFi.localIP() : WiFi.softAPIP(); }

int rssi() { return station ? WiFi.RSSI() : 0; }

String placeId() {
  if (!station) {
    return "ap";
  }
  if (PASS_LOCK_TO_BSSID) {
    return WiFi.BSSIDstr();
  }
  return String("ssid:") + WiFi.SSID();
}

} // namespace net
