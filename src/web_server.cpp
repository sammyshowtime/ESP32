#include "web_server.h"

#include <WebServer.h>

#include "config.h"
#include "index_html.h"
#include "net.h"
#include "outputs.h"

namespace {

WebServer server(HTTP_PORT);

const char *const kTokenHeader = "X-Auth-Token";

bool authRequired() { return strlen(API_TOKEN) > 0; }

// Accepts the token from either the X-Auth-Token header or a ?token= query
// parameter, so both fetch() and curl are easy to use.
bool authorize() {
  if (!authRequired()) {
    return true;
  }
  const String supplied = server.hasHeader(kTokenHeader)
                              ? server.header(kTokenHeader)
                              : server.arg("token");
  if (supplied == API_TOKEN) {
    return true;
  }
  server.send(401, "application/json",
              "{\"error\":\"unauthorized\"}");
  return false;
}

void sendError(int code, const char *message) {
  server.send(code, "application/json",
              String("{\"error\":\"") + message + "\"}");
}

String stateJson() {
  String json;
  json.reserve(512);
  json += "{\"hostname\":\"" HOSTNAME "\"";
  json += ",\"mode\":\"" + net::modeName() + "\"";
  json += ",\"ssid\":\"" + net::ssid() + "\"";
  json += ",\"ip\":\"" + net::ip().toString() + "\"";
  json += ",\"rssi\":" + String(net::rssi());
  json += ",\"uptime_s\":" + String(millis() / 1000);
  json += ",\"dimmer\":" + String(outputs::dimmer());
  json += ",\"dimmer_max\":" + String(DIMMER_MAX);
  json += ",\"switches\":[";
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    if (i) {
      json += ',';
    }
    json += "{\"index\":" + String(i);
    json += ",\"name\":\"" + String(outputs::switchName(i)) + "\"";
    json += ",\"pin\":" + String(SWITCH_PINS[i]);
    json += ",\"on\":";
    json += outputs::switchState(i) ? "true" : "false";
    json += '}';
  }
  json += "]}";
  return json;
}

void sendState() { server.send(200, "application/json", stateJson()); }

// Reads a truthy/falsy query value. Returns false when the value is missing or
// unrecognised, so callers can reject the request.
bool parseBool(const String &raw, bool *out) {
  if (raw == "1" || raw.equalsIgnoreCase("true") || raw.equalsIgnoreCase("on")) {
    *out = true;
    return true;
  }
  if (raw == "0" || raw.equalsIgnoreCase("false") ||
      raw.equalsIgnoreCase("off")) {
    *out = false;
    return true;
  }
  return false;
}

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleState() {
  if (!authorize()) {
    return;
  }
  sendState();
}

void handleSwitch() {
  if (!authorize()) {
    return;
  }
  if (!server.hasArg("index") || !server.hasArg("value")) {
    return sendError(400, "index and value are required");
  }

  const long index = server.arg("index").toInt();
  if (index < 0 || index >= SWITCH_COUNT) {
    return sendError(400, "index out of range");
  }

  const String value = server.arg("value");
  bool on = false;
  if (value.equalsIgnoreCase("toggle")) {
    outputs::toggleSwitch(static_cast<uint8_t>(index));
  } else if (parseBool(value, &on)) {
    outputs::setSwitch(static_cast<uint8_t>(index), on);
  } else {
    return sendError(400, "value must be 1, 0 or toggle");
  }
  sendState();
}

void handleAll() {
  if (!authorize()) {
    return;
  }
  bool on = false;
  if (!parseBool(server.arg("value"), &on)) {
    return sendError(400, "value must be 1 or 0");
  }
  outputs::setAllSwitches(on);
  sendState();
}

void handleDimmer() {
  if (!authorize()) {
    return;
  }
  if (!server.hasArg("value")) {
    return sendError(400, "value is required");
  }
  const long duty = server.arg("value").toInt();
  if (duty < 0 || duty > DIMMER_MAX) {
    return sendError(400, "value out of range");
  }
  outputs::setDimmer(static_cast<uint16_t>(duty));
  sendState();
}

void handleNotFound() { sendError(404, "not found"); }

} // namespace

namespace web {

void begin() {
  const char *headers[] = {kTokenHeader};
  server.collectHeaders(headers, 1);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/switch", HTTP_POST, handleSwitch);
  server.on("/api/all", HTTP_POST, handleAll);
  server.on("/api/dimmer", HTTP_POST, handleDimmer);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.printf("[web] listening on port %d\n", HTTP_PORT);
  if (authRequired()) {
    Serial.println("[web] API token required");
  }
}

void loop() { server.handleClient(); }

} // namespace web
