// Central configuration for the ESP32 remote control.
// Everything you are likely to want to change lives here.
#pragma once

#include <Arduino.h>

// --- Credentials ------------------------------------------------------------
// Copy include/secrets.h.example to include/secrets.h and edit it. If that file
// is missing the build still succeeds and the board comes up in access-point
// mode, so a fresh clone is always flashable.
#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef API_TOKEN
#define API_TOKEN "" // empty disables authentication
#endif

// --- Network ----------------------------------------------------------------
#define HOSTNAME "esp32-remote" // reachable as http://esp32-remote.local
#define AP_SSID "ESP32-Remote"
#define AP_PASSWORD "remote1234" // WPA2 requires at least 8 characters
#define HTTP_PORT 80

#define WIFI_CONNECT_TIMEOUT_MS 15000UL // give up on the router after this long
#define WIFI_RETRY_INTERVAL_MS 30000UL  // then retry in the background this often

// --- Outputs ----------------------------------------------------------------
#define SWITCH_COUNT 4

#if CONFIG_IDF_TARGET_ESP32C3
static const uint8_t SWITCH_PINS[SWITCH_COUNT] = {2, 3, 4, 5};
#define DIMMER_PIN 6
#define STATUS_LED_PIN 8
#elif CONFIG_IDF_TARGET_ESP32S3
static const uint8_t SWITCH_PINS[SWITCH_COUNT] = {4, 5, 6, 7};
#define DIMMER_PIN 15
#define STATUS_LED_PIN 48
#else
static const uint8_t SWITCH_PINS[SWITCH_COUNT] = {23, 22, 21, 19};
#define DIMMER_PIN 18
#define STATUS_LED_PIN 2
#endif

// Names shown in the web UI, one per switch.
static const char *const SWITCH_NAMES[SWITCH_COUNT] = {
    "Channel 1", "Channel 2", "Channel 3", "Channel 4"};

// Most relay breakout boards pull the coil in when the input is driven LOW.
// Set this to true for those; leave it false when driving LEDs or MOSFETs.
#define SWITCH_ACTIVE_LOW false

// PWM output, e.g. an LED dimmer or a motor speed control.
#define DIMMER_FREQ_HZ 5000
#define DIMMER_RESOLUTION_BITS 8
#define DIMMER_LEDC_CHANNEL 0
#define DIMMER_MAX ((1 << DIMMER_RESOLUTION_BITS) - 1)

// Restore the last switch/dimmer state from flash after a reboot or power cut.
#define RESTORE_STATE_ON_BOOT true

// --- Sticker passes ---------------------------------------------------------
// Printable QR stickers that let a guest use the remote. Only meaningful when
// API_TOKEN is set - without it the remote is open to everyone anyway.
#define PASS_MAX 32 // stored passes; spent one-time passes are recycled first

// A location-locked sticker only works while the remote sits on the network it
// was printed on. true pins it to that exact router (its BSSID), so a copy of
// your network name elsewhere doesn't count. Set false to match on the network
// name instead - use that with mesh Wi-Fi, where the remote may hop between
// nodes that each have their own BSSID.
#define PASS_LOCK_TO_BSSID true
