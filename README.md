# Tamaboard

A virtual pet on a Heltec Wireless Paper — ESP32-S3 with a 2.13" e-ink display. Connect your phone to the board's Wi-Fi AP, open the companion web UI, and you've got a wireless touchpad + key surface that sends HID reports over BLE to any paired host.

The pet has needs. Using it as a keyboard drains its energy and makes it hungry, so you actually have to take care of it.

## What you need

- Heltec Wireless Paper v1.2 (ESP32-S3FN8, E0213A367 e-ink panel)
- LiPo battery — technically optional, but required for stable flashing
- USB-C cable for flashing/serial (CP2102 bridge — serial only, not OTG)

## Building and flashing

You need pioarduino's PlatformIO, not Homebrew's. The pioarduino platform ships C extensions compiled for Python 3.11 and they won't run under Homebrew's 3.14.

```sh
curl -fsSL -o get-platformio.py \
  https://raw.githubusercontent.com/pioarduino/pioarduino-core-installer/pioarduino/get-platformio.py
python3 get-platformio.py
```

Then use the wrappers in `bin/` instead of calling `pio` directly:

```sh
# Build only
./bin/pio run

# Flash firmware + web UI (most common)
./bin/flash --fs

# Flash firmware only
./bin/flash

# Flash web UI only (after editing data/index.html)
./bin/flash --fs-only

# Serial monitor
./bin/pio device monitor
```

Attach the LiPo before flashing. USB-C alone can't reliably supply enough current during download mode and you'll get sync errors from esptool.

If a previous firmware grabs the UART before esptool can sync: hold **PRG**, tap **RST**, release **PRG** to enter download mode manually, then `./bin/pio run -t erase`.

## Using it

### On the device (one button)

| Button | Context | Action |
|--------|---------|--------|
| Short press | Root carousel | Next page (Home → Keyboard → Settings) |
| Long press | Root — Home | Open Care menu (Feed / Play / Sleep) |
| Long press | Root — Keyboard / Settings | Enter page (header inverts) |
| Short press | Inside page | Next list item |
| Long press | Inside page | Execute selected action |
| Long press | "Back" item | Return to carousel |
| Any press | Screen paused | Unpause (does not navigate) |

The screen auto-pauses after 30 s of inactivity to protect the e-ink. The header shows `PAUSED | HH:MM`. Press any button to wake it.

The onboard LED blinks to signal events: 1× state change, 2× BLE connected, 3× BLE failed, rapid during pairing.

### Companion web UI

Connect to the **Tamaboard** Wi-Fi AP (password: `tamaboard`) and open `http://tamaboard.local/` (or `http://192.168.4.1/` on devices without mDNS). From there:

- Live pet stats — hunger, mood, energy, battery %
- Feed / play / sleep actions
- BLE host slot management — pair, connect, forget (up to 3 hosts)
- Touchpad + key grid when in Keyboard mode

Debug page at `http://tamaboard.local/debug` — uptime, heap, reset reason, pet stats, BLE status.

## Pet states

| State | What happens |
|-------|-------------|
| `IDLE` | Hunger rises slowly over time |
| `EAT` | Hunger drops; returns to IDLE when full |
| `PLAY` | Mood rises, energy drops; returns to IDLE |
| `SLEEP` | Energy recovers; WiFi + BLE off; screen locks; PRG wakes |
| `KEYBOARD` | BLE keyboard/mouse active; energy drains, hunger rises |

Stats persist across reboots (NVS). Only IDLE and SLEEP survive a power cycle — EAT/PLAY/KEYBOARD reset to IDLE.

## BLE keyboard

From the Keyboard page, long-press a slot to Pair, Connect, or Forget. Up to 3 hosts bonded at once. During pairing a 6-digit PIN appears on the e-ink — enter it on the host when prompted.

Once connected, the companion web UI's touchpad and key grid send HID events over BLE to the paired host.

## Settings

From the Settings page (long-press at root when on Settings):

- **Airplane mode** — disables WiFi + BLE. Persists across reboots.
- **LED** — enable or disable the onboard LED blinker.

## How it's put together

The WebSocket is the only API — no REST, no polling. The board pushes JSON to all connected clients on every state change. Commands come in as JSON frames (`action`, `key`, `move`, `click`, `slot`, `clock`).

```
Phone browser ──WiFi WebSocket──▶ ESP32-S3
                                      │
                              ┌───────┴──────────────┐
                              ▼                       ▼
                         Controller (Core 0)    uiTask (Core 1)
                              │                  (e-ink render)
                              ▼
                    PetFSM (pure state)
                    BleHid (NimBLE)
                    DeviceSlots (NVS)
```

`PetFSM` is pure — no side effects. Everything dirty (HID lifecycle, WiFi/BLE on/off, sleep) lives in `main.cpp`'s `onFsmStateChange`. Navigation and state mutations run on Core 0; all e-ink rendering runs on Core 1 (`uiTask`), so Core 0 never blocks on display I/O. See [CLAUDE.md](CLAUDE.md) for full architecture notes, pin details, and build conventions.

## Project layout

```
src/
├── main.cpp          # setup/loop, uiTask, side-effect runner
├── pet/              # PetFSM (pure), PetState
├── hid/              # HidBackend interface, BleHid, UsbHid (stub)
├── io/               # Button, Battery, Clock, DeviceSlots, Led, PetStore, Settings
├── net/              # WebUI (HTTP+WS), StateBroadcaster, DebugPage
├── routine/          # Routine base, RoutineManager, BatteryRoutine
├── app/              # Controller (single command sink)
└── ui/               # Canvas, EinkDisplay, PageStack, pages
data/
└── index.html        # companion web app (vanilla JS, no framework)
```

## License

MIT — see [LICENSE](LICENSE).
