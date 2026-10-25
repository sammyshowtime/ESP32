#include "web_server.h"

#include <WebServer.h>

#include "config.h"
#include "index_html.h"
#include "net.h"
#include "outputs.h"
#include "passes.h"

namespace {

WebServer server(HTTP_PORT);

const char *const kTokenHeader = "X-Auth-Token";
const char *const kPassHeader = "X-Pass";

bool authRequired() { return strlen(API_TOKEN) > 0; }

void sendError(int code, const char *message) {
  server.send(code, "application/json",
              String("{\"error\":\"") + message + "\"}");
}

// Admins hold the API token. Guests hold a sticker pass and can use the
// controls but not manage passes.
enum class Role { Denied, Guest, Admin };

Role role = Role::Denied;
String requestPass; // the pass on the current request, when role is Guest

String credential(const char *header, const char *arg) {
  return server.hasHeader(header) ? server.header(header) : server.arg(arg);
}

// Accepts the token from either the X-Auth-Token header or a ?token= query
// parameter, so both fetch() and curl are easy to use; likewise a sticker pass
// from X-Pass or ?pass=. A request carrying a pass is always judged on that
// pass, so stickers behave the same whether or not API_TOKEN is set.
bool authorize(bool adminOnly = false) {
  role = Role::Denied;
  requestPass = credential(kPassHeader, "pass");

  if (requestPass.length()) {
    const passes::Check result = passes::check(requestPass);
    if (result != passes::Check::Ok) {
      sendError(403, passes::checkMessage(result));
      return false;
    }
    role = Role::Guest;
  } else if (!authRequired() ||
             credential(kTokenHeader, "token") == API_TOKEN) {
    role = Role::Admin;
  } else {
    sendError(401, "unauthorized");
    return false;
  }

  if (adminOnly && role != Role::Admin) {
    sendError(403, "admin only");
    return false;
  }
  return true;
}

// A one-time pass is spent by the first control action it makes, not by
// viewing state, so a guest can see the remote before using their turn.
void spendPass() {
  if (role == Role::Guest) {
    passes::spend(requestPass);
  }
}

// Labels are user-supplied, so escape them before they go into JSON.
String jsonString(const char *raw) {
  String out = "\"";
  for (const char *c = raw; *c; c++) {
    if (*c == '"' || *c == '\\') {
      out += '\\';
      out += *c;
    } else if (static_cast<uint8_t>(*c) < 0x20) {
      out += ' ';
    } else {
      out += *c;
    }
  }
  return out + '"';
}

String passJson(const passes::Pass &p) {
  String json = "{\"code\":\"" + String(p.code) + "\"";
  json += ",\"label\":" + jsonString(p.label);
  json += ",\"locked\":";
  json += p.locked ? "true" : "false";
  json += ",\"once\":";
  json += p.once ? "true" : "false";
  json += ",\"used\":";
  json += p.used ? "true" : "false";
  json += ",\"here\":";
  json += passes::isHere(p) ? "true" : "false";
  return json + '}';
}

String stateJson() {
  String json;
  json.reserve(768);
  json += "{\"hostname\":\"" HOSTNAME "\"";
  json += ",\"mode\":\"" + net::modeName() + "\"";
  json += ",\"ssid\":\"" + net::ssid() + "\"";
  json += ",\"ip\":\"" + net::ip().toString() + "\"";
  json += ",\"rssi\":" + String(net::rssi());
  json += ",\"uptime_s\":" + String(millis() / 1000);
  json += ",\"dimmer\":" + String(outputs::dimmer());
  json += ",\"dimmer_max\":" + String(DIMMER_MAX);
  json += ",\"auth_required\":";
  json += authRequired() ? "true" : "false";
  json += ",\"access\":";
  json += role == Role::Admin ? "\"admin\"" : "\"guest\"";
  if (role == Role::Guest) {
    const passes::Pass *p = passes::find(requestPass);
    if (p) {
      json += ",\"pass\":" + passJson(*p);
    }
  }
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
  spendPass();
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
  spendPass();
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
  spendPass();
  sendState();
}

void sendPasses(const String &created) {
  String json = "{\"place\":\"" + net::placeId() + "\",\"passes\":[";
  for (uint8_t i = 0; i < passes::count(); i++) {
    if (i) {
      json += ',';
    }
    json += passJson(passes::at(i));
  }
  json += "],\"created\":[" + created + "]}";
  server.send(200, "application/json", json);
}

void handleListPasses() {
  if (!authorize(true)) {
    return;
  }
  sendPasses("");
}

// POST /api/passes?count=N&lock=1|0&once=1|0&label=...
// Both settings default to "lock to this location, reusable".
void handleCreatePasses() {
  if (!authorize(true)) {
    return;
  }
  bool locked = true;
  bool once = false;
  if (server.hasArg("lock") && !parseBool(server.arg("lock"), &locked)) {
    return sendError(400, "lock must be 1 or 0");
  }
  if (server.hasArg("once") && !parseBool(server.arg("once"), &once)) {
    return sendError(400, "once must be 1 or 0");
  }
  const long n = server.hasArg("count") ? server.arg("count").toInt() : 1;
  if (n < 1 || n > PASS_MAX) {
    return sendError(400, "count out of range");
  }

  String created;
  for (long i = 0; i < n; i++) {
    passes::Pass p;
    if (!passes::create(server.arg("label"), locked, once, &p)) {
      if (i == 0) {
        return sendError(507, "pass storage full - revoke some stickers");
      }
      break; // hand back the ones that fit
    }
    if (i) {
      created += ',';
    }
    created += passJson(p);
  }
  sendPasses(created);
}

void handleRevokePass() {
  if (!authorize(true)) {
    return;
  }
  if (!passes::revoke(server.arg("code"))) {
    return sendError(404, "no such pass");
  }
  sendPasses("");
}

void handleNotFound() { sendError(404, "not found"); }

} // namespace

namespace web {

void begin() {
  const char *headers[] = {kTokenHeader, kPassHeader};
  server.collectHeaders(headers, 2);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/switch", HTTP_POST, handleSwitch);
  server.on("/api/all", HTTP_POST, handleAll);
  server.on("/api/dimmer", HTTP_POST, handleDimmer);
  server.on("/api/passes", HTTP_GET, handleListPasses);
  server.on("/api/passes", HTTP_POST, handleCreatePasses);
  server.on("/api/passes", HTTP_DELETE, handleRevokePass);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.printf("[web] listening on port %d\n", HTTP_PORT);
  if (authRequired()) {
    Serial.println("[web] API token required");
  }
}

void loop() { server.handleClient(); }

} // namespace web
