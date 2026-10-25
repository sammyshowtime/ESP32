#include "passes.h"

#include <Preferences.h>
#include <esp_random.h>

#include "config.h"
#include "net.h"

namespace {

Preferences prefs;
passes::Pass table[PASS_MAX];
uint8_t used = 0; // number of live entries at the front of table
uint32_t nextSeq = 1;

// No 0/O, 1/I/L, so a code copied off a sticker by hand still works.
const char kAlphabet[] = "23456789ABCDEFGHJKMNPQRSTUVWXYZ";

// The whole table is rewritten on every change. Passes change rarely (print,
// revoke, a one-time pass being spent), so flash wear is not a concern.
void save() {
  prefs.putBytes("table", table, sizeof(passes::Pass) * used);
  prefs.putUChar("count", used);
}

int indexOf(const String &code) {
  for (uint8_t i = 0; i < used; i++) {
    if (code.equalsIgnoreCase(table[i].code)) {
      return i;
    }
  }
  return -1;
}

void removeAt(uint8_t index) {
  for (uint8_t i = index; i + 1 < used; i++) {
    table[i] = table[i + 1];
  }
  used--;
}

// When the table is full, the oldest spent one-time pass makes room: its
// sticker can never work again anyway.
bool makeRoom() {
  if (used < PASS_MAX) {
    return true;
  }
  int oldest = -1;
  for (uint8_t i = 0; i < used; i++) {
    if (table[i].used && (oldest < 0 || table[i].seq < table[oldest].seq)) {
      oldest = i;
    }
  }
  if (oldest < 0) {
    return false;
  }
  removeAt(oldest);
  return true;
}

void newCode(char *out) {
  do {
    for (uint8_t i = 0; i < 8; i++) {
      out[i] = kAlphabet[esp_random() % (sizeof(kAlphabet) - 1)];
    }
    out[8] = '\0';
  } while (indexOf(out) >= 0);
}

} // namespace

namespace passes {

void begin() {
  prefs.begin("passes", false);
  used = prefs.getUChar("count", 0);
  if (used > PASS_MAX ||
      prefs.getBytesLength("table") != sizeof(Pass) * used) {
    used = 0; // layout changed or corrupt; start fresh
  } else if (used) {
    prefs.getBytes("table", table, sizeof(Pass) * used);
  }
  for (uint8_t i = 0; i < used; i++) {
    if (table[i].seq >= nextSeq) {
      nextSeq = table[i].seq + 1;
    }
  }
  Serial.printf("[passes] %u sticker pass(es) loaded\n", used);
}

bool create(const String &label, bool locked, bool once, Pass *out) {
  if (!makeRoom()) {
    return false;
  }
  Pass &p = table[used];
  memset(&p, 0, sizeof(p));
  newCode(p.code);
  strlcpy(p.label, label.c_str(), sizeof(p.label));
  if (locked) {
    strlcpy(p.place, net::placeId().c_str(), sizeof(p.place));
  }
  p.locked = locked;
  p.once = once;
  p.seq = nextSeq++;
  used++;
  save();
  if (out) {
    *out = p;
  }
  return true;
}

bool revoke(const String &code) {
  const int i = indexOf(code);
  if (i < 0) {
    return false;
  }
  removeAt(i);
  save();
  return true;
}

Check check(const String &code) {
  const int i = indexOf(code);
  if (i < 0) {
    return Check::Unknown;
  }
  const Pass &p = table[i];
  if (p.once && p.used) {
    return Check::Used;
  }
  if (!isHere(p)) {
    return Check::WrongPlace;
  }
  return Check::Ok;
}

void spend(const String &code) {
  const int i = indexOf(code);
  if (i < 0 || !table[i].once || table[i].used) {
    return;
  }
  table[i].used = true;
  save();
}

const Pass *find(const String &code) {
  const int i = indexOf(code);
  return i < 0 ? nullptr : &table[i];
}

uint8_t count() { return used; }

const Pass &at(uint8_t index) { return table[index]; }

bool isHere(const Pass &pass) {
  return !pass.locked || net::placeId() == pass.place;
}

const char *checkMessage(Check result) {
  switch (result) {
  case Check::Ok:
    return "ok";
  case Check::Used:
    return "this one-time sticker has already been used";
  case Check::WrongPlace:
    return "this sticker only works where it was printed";
  case Check::Unknown:
  default:
    return "this sticker is not valid";
  }
}

} // namespace passes
