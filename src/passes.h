// Sticker passes: short codes printed as QR stickers that give a guest access
// to the remote without handing out the API token.
//
// Each pass carries two independent settings:
//   locked - only works while the remote is on the same network it was on when
//            the sticker was printed (see net::placeId()).
//   once   - works for a single control action, then is spent.
#pragma once

#include <Arduino.h>

namespace passes {

struct Pass {
  char code[9];   // 8 characters plus terminator
  char label[25]; // printed under the QR code
  char place[40]; // where it was printed, when locked
  bool locked;
  bool once;
  bool used;
  uint32_t seq; // creation order, oldest first
};

enum class Check {
  Ok,
  Unknown,    // no such code, or it was revoked
  Used,       // one-time pass already spent
  WrongPlace, // location-locked pass used somewhere else
};

// Loads saved passes from flash.
void begin();

// Creates a pass bound to the current location when locked. Returns false
// when storage is full of passes that are still live.
bool create(const String &label, bool locked, bool once, Pass *out);

// Deletes a pass so its sticker stops working. Returns false if unknown.
bool revoke(const String &code);

// Validates a code without spending it.
Check check(const String &code);

// Marks a one-time pass as spent. No-op for reusable passes.
void spend(const String &code);

// Returns the stored pass for a code, or nullptr.
const Pass *find(const String &code);

uint8_t count();
const Pass &at(uint8_t index);

// True when a locked pass would be accepted where the remote is right now.
bool isHere(const Pass &pass);

const char *checkMessage(Check result);

} // namespace passes
