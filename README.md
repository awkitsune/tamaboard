# Tamaboard

I wanted a physical virtual pet that could also double as a BLE keyboard and mouse. So I put one on a Heltec Wireless Paper — an ESP32-S3 with a 2.13" e-ink display. Connect your phone to the board's Wi-Fi AP, open the companion web UI, and you've got a wireless touchpad + key surface that sends HID reports over BLE to any paired host.

The pet has needs though. Using it as a keyboard drains its energy and makes it hungry, so you actually have to take care of it.

## What you need

- Heltec Wireless Paper v1.2 (ESP32-S3FN8, E0213A367 e-ink panel)
- LiPo battery — technically optional, but you'll need it for stable flashing
- USB-C cable for flashing/serial (goes through a CP2102 bridge, not OTG — just serial)

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

Attach the LiPo before flashing. USB-C alone can't reliably supply enough current through the download-mode current spikes and you'll get sync errors from esptool.

If a previous firmware grabs the UART before esptool can sync: hold **PRG**, tap **RST**, release **PRG** to enter download mode manually, then `./bin/pio run -t erase`.

## Using it

### On the device (one button)

| Button | Context | Action |
|--------|---------|--------|
| Short press | Root carousel | Next page (Home → Keyboard → Settings) |
| Long press | Root carousel — Home | Open Care menu (Feed / Play / Sleep); action returns to Home |
| Long press | Root carousel — other | Enter page (header inverts to signal "inside") |
| Short press | Inside page | Next list item |
| Long press | Inside page | Execute selected action |
| Long press | "Back" item | Return to carousel |

### Companion web UI

Connect to the **Tamaboard** Wi-Fi AP (password: `tamaboard`) and open `http://tamaboard.local/` in a browser (or `http://192.168.4.1/` if your device doesn't resolve mDNS). From there you can:

- See live pet stats (hunger, mood, energy, battery)
- Feed, play, or put the pet to sleep
- Manage BLE host slots (pair, connect, forget)
- Use the touchpad + key grid when the pet is in Keyboard mode

There's also a debug page at `http://tamaboard.local/debug` — uptime, heap, WiFi IP, reset reason, pet stats, BLE status.

## Pet states

| State | What happens |
|-------|-------------|
| `IDLE` | Hunger rises slowly over time |
| `EAT` | Hunger drops; returns to IDLE when full |
| `PLAY` | Mood rises, energy drops; returns to IDLE when exhausted |
| `SLEEP` | Energy recovers; WiFi + BLE off; screen locks; PRG button wakes |
| `KEYBOARD` | Acts as BLE keyboard/mouse; energy drains, hunger rises |

Stats persist across reboots via NVS. Only IDLE and SLEEP survive a power cycle — EAT/PLAY/KEYBOARD reset to IDLE.

## BLE keyboard

From the Keyboard page, long-press a slot to Pair, Connect, or Forget. Up to 3 hosts can be bonded at once. During pairing, a 6-digit PIN shows up on the e-ink display — enter it on the host when prompted.

Once connected, the companion web UI's touchpad and key grid send HID events over BLE to the paired host.

## How it's put together

The WebSocket is the only API — no REST, no polling. The board pushes JSON to all connected clients whenever state changes. Commands come in as JSON frames (`action`, `key`, `move`, `click`, `slot`, `clock`).

```
Phone browser ──WiFi WebSocket──▶ ESP32-S3
                                      │
                              ┌───────┴────────┐
                              ▼                ▼
                         Controller        PageStack
                              │           (e-ink UI)
                              ▼
                    PetFSM (pure state)
                         BleHid (NimBLE)
                         DeviceSlots (NVS)
```

`PetFSM` is pure — no side effects. Everything dirty (HID lifecycle, WiFi/BLE on/off, sleep) lives in `main.cpp`'s `onFsmStateChange` so it's easy to find. See [CLAUDE.md](CLAUDE.md) for the full architecture notes, pin details, and build conventions.

## Project layout

```
src/
├── main.cpp          # setup/loop + side-effect runner
├── pet/              # PetFSM (pure), PetState
├── hid/              # HidBackend interface, BleHid, UsbHid (stub)
├── io/               # Button, Battery, Clock, DeviceSlots, PetStore, EventLog
├── net/              # WebUI (HTTP+WS), StateBroadcaster
├── app/              # Controller (single command sink)
└── ui/               # Canvas, EinkDisplay, PageStack, pages
data/
└── index.html        # companion web app (vanilla JS, no framework)
```

## License

MIT — see [LICENSE](LICENSE).
