# ESP32 Remote Control

Turn an ESP32 into a Wi-Fi remote control. The board joins your network, serves a
responsive web page, and exposes a small JSON API for switching four output
channels and driving a PWM dimmer. No extra hardware is required to try it —
the four channels and the dimmer are plain GPIOs you can wire to relays, LEDs,
MOSFETs or a motor driver whenever you're ready.

```
phone / laptop  ──HTTP──>  ESP32  ──GPIO──>  relays, LEDs, motor driver
                            └── remembers its state across reboots
```

## Features

- **Web UI** served from flash — responsive, works in light and dark mode, no
  internet or CDN needed.
- **JSON API** for scripting from `curl`, Home Assistant, Node-RED or anything
  else that speaks HTTP.
- **Access-point fallback.** If the router is unreachable at boot the ESP32
  raises its own network so you can always reach it, and keeps retrying your
  network in the background.
- **State survives a power cut.** Switch and dimmer positions are written to
  NVS (batched, so dragging the slider doesn't wear out the flash).
- **mDNS** — reachable at `http://esp32-remote.local`.
- **Optional token auth** on the API.
- **Status LED** heartbeat: slow blink when joined to your Wi-Fi, fast blink
  while on the fallback access point.

## Getting started

1. Install [PlatformIO](https://platformio.org/install) (`pip install platformio`).
2. Set your credentials:

   ```sh
   cp include/secrets.h.example include/secrets.h
   # then edit include/secrets.h
   ```

   `include/secrets.h` is gitignored. Skipping this step is fine for a first
   flash — the board will come up in access-point mode.

3. Build and flash:

   ```sh
   pio run -t upload      # ESP32 dev board (default)
   pio device monitor     # watch the boot log for the IP address
   ```

   Other boards are preconfigured:

   ```sh
   pio run -e esp32-s3-devkitc-1 -t upload
   pio run -e esp32-c3-devkitm-1 -t upload
   ```

4. Open `http://esp32-remote.local` — or the IP printed on the serial monitor.

If the board couldn't reach your router, join the Wi-Fi network **ESP32-Remote**
(password `remote1234`) and browse to `http://192.168.4.1`.

## Wiring

Default pins, configurable in [`include/config.h`](include/config.h):

| Function  | ESP32 | ESP32-S3 | ESP32-C3 |
| --------- | ----- | -------- | -------- |
| Channel 1 | 23    | 4        | 2        |
| Channel 2 | 22    | 5        | 3        |
| Channel 3 | 21    | 6        | 4        |
| Channel 4 | 19    | 7        | 5        |
| Dimmer    | 18    | 15       | 6        |
| Status LED| 2     | 48       | 8        |

Driving LEDs directly is fine through a series resistor. For anything mains-
powered use an opto-isolated relay module — and note that most relay boards are
active-low, so set `SWITCH_ACTIVE_LOW` to `true` in `config.h`.

> Mains voltage is dangerous. If you aren't confident wiring it, drive a
> low-voltage load instead.

## API

Every endpoint returns the full device state, so a client never needs a second
round trip to refresh its UI.

| Method | Path                            | Purpose                          |
| ------ | ------------------------------- | -------------------------------- |
| `GET`  | `/`                             | Web UI                           |
| `GET`  | `/api/state`                    | Current state                    |
| `POST` | `/api/switch?index=N&value=V`   | `V` is `1`, `0` or `toggle`      |
| `POST` | `/api/all?value=V`              | All channels at once             |
| `POST` | `/api/dimmer?value=0..255`      | PWM duty cycle                   |

```sh
curl http://esp32-remote.local/api/state
curl -X POST "http://esp32-remote.local/api/switch?index=0&value=toggle"
curl -X POST "http://esp32-remote.local/api/all?value=0"
curl -X POST "http://esp32-remote.local/api/dimmer?value=128"
```

Response:

```json
{
  "hostname": "esp32-remote",
  "mode": "sta",
  "ssid": "home-wifi",
  "ip": "192.168.1.50",
  "rssi": -58,
  "uptime_s": 412,
  "dimmer": 128,
  "dimmer_max": 255,
  "switches": [
    { "index": 0, "name": "Channel 1", "pin": 23, "on": true },
    { "index": 1, "name": "Channel 2", "pin": 22, "on": false },
    { "index": 2, "name": "Channel 3", "pin": 21, "on": false },
    { "index": 3, "name": "Channel 4", "pin": 19, "on": false }
  ]
}
```

### Authentication

Set `API_TOKEN` in `include/secrets.h` to something non-empty and every
`/api/*` request must then carry it, as either a header or a query parameter:

```sh
curl -H "X-Auth-Token: hunter2" http://esp32-remote.local/api/state
curl "http://esp32-remote.local/api/state?token=hunter2"
```

The web UI prompts for the token on the first unauthorised request and
remembers it in `localStorage`. This is plain HTTP on your LAN — the token
keeps casual visitors out, it is not a substitute for network security.

## Customising

Everything worth changing is in [`include/config.h`](include/config.h): pin
assignments, channel names, how many channels there are, active-low relay
inversion, PWM frequency and resolution, hostname, AP credentials, and whether
state is restored on boot. Adding a fifth channel means adding a pin and a name
and bumping `SWITCH_COUNT` — the UI builds itself from `/api/state`.

## Project layout

```
include/config.h        all user-tunable settings
include/secrets.h       your Wi-Fi credentials (gitignored)
src/main.cpp            setup/loop and the status LED
src/net.*               Wi-Fi, AP fallback, mDNS
src/outputs.*           GPIO/PWM state and NVS persistence
src/web_server.*        HTTP routes and JSON
src/index_html.h        the web UI, embedded in flash
```
