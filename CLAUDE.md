# Tamaboard — Project Context

A virtual pet running on a **Heltec Wireless Paper v1.2** (ESP32-S3 + e-ink).
One of the pet's "activities" is acting as a **BLE keyboard/mouse**: a phone
opens the companion web UI to drive a touchpad + key surface; the board emits
HID reports over BLE to a paired host (Mac/PC/phone).

This file is the durable project memory. Read it before making changes. Treat the
hardware facts and stack decisions below as settled unless told otherwise.

---

## Hardware facts (board-specific, easy to get wrong)

- **MCU:** ESP32-S3FN8 — dual-core Xtensa LX7, 8 MB flash, **no PSRAM**, 512 KB SRAM.
  WiFi + BLE (BLE only, no Bluetooth Classic). Native USB OTG peripheral.
- **Radio:** SX1262 LoRa on board. **Not used** in this project.
- **Display:** 2.13" mono e-ink, 250 x 122 px. Panel on **v1.2 is E0213A367**
  (use `EInkDisplay_WirelessPaperV1_2` — the friendly alias — when instantiating
  the heltec-eink-modules class). The older LCMEN2R13EFC1 / DEPG0213BNS800
  drivers will silently hang the library forever on the BUSY pin if you pick
  one of those by mistake. Power to display + peripherals is an
  **active-low switch on GPIO45** (Vext).
- **USB-C port goes through a CP2102 USB-UART bridge — it CANNOT do USB HID.**
  Plugging USB-C into a host gives a serial port only. Use it for power, flashing,
  and `Serial` logs.
- **Native USB (for HID) is on the chip pins GPIO20 = D+ and GPIO19 = D-.** These are
  broken out on the board header. To act as a USB HID device you must hand-wire a USB
  plug to these pins. VERIFY the exact pad positions against this board's pinout before
  soldering.

## Wiring for USB HID

Solder to a USB plug going into the target computer:

| Board pin | USB line | Wire (typical) |
|-----------|----------|----------------|
| GPIO20    | D+       | green          |
| GPIO19    | D-       | white          |
| GND       | GND      | black          |

Power the board from its own USB-C or battery. If the host won't enumerate, the OTG
controller may need VBUS sensing — feed host 5V to the chip's VBUS-sense path via a
divider. Test enumeration early; this is the riskiest hardware step.

---

## Stack (decided)

- **Build:** PlatformIO Core (CLI). Dev host is macOS / fish shell.
- **Platform:** the **pioarduino** fork of platform-espressif32 — NOT the stock
  `espressif32` platform. We need Arduino-ESP32 core v3.x (IDF 5.5.x) for the modern
  TinyUSB stack; stock PlatformIO lagged on the v3 core.
- **E-ink:** `todd-herbert/heltec-eink-modules` — NOT GxEPD2. The v1.2 panel
  (E0213A367) is unsupported in GxEPD2. This library targets the Wireless Paper,
  draws via Adafruit-GFX style, handles the GPIO45 power switch, and pages rendering
  (so no-PSRAM is fine).
- **USB HID:** TinyUSB, built into core v3 (`USB.h`, `USBHIDKeyboard.h`, `USBHIDMouse.h`).
  Drives the native USB peripheral on GPIO19/20. **Currently stubbed** — GPIO19/20 not
  wired. `UsbHid` is a no-op behind the `HidBackend` interface.
- **BLE HID:** `h2zero/NimBLE-Arduino` for a HID-over-GATT service.
- **Web layer:** `ESP32Async/ESPAsyncWebServer` + `ESP32Async/AsyncTCP`, static UI from
  LittleFS, control via WebSocket.
- **Frontend:** a single hand-written `data/index.html` (vanilla JS, pointer events,
  WebSocket). Do NOT ship an Angular/framework runtime off MCU flash for a UI this small.

## platformio.ini

```ini
[env:tamaboard]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.36-1/platform-espressif32.zip
board = heltec_wifi_lora_32_V3   ; same MCU/flash as Wireless Paper; no dedicated def
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
upload_speed = 460800

build_flags =
    -D CORE_DEBUG_LEVEL=3
    -D WIRELESS_PAPER      ; enables heltec-eink-modules pin presets for this board

board_build.filesystem = littlefs

lib_deps =
    https://github.com/todd-herbert/heltec-eink-modules
    h2zero/NimBLE-Arduino
    ESP32Async/ESPAsyncWebServer
    ESP32Async/AsyncTCP
```

> Pin to the latest pioarduino release tag rather than the URL above.
> The USB HID build flags (`ARDUINO_USB_MODE=0`, `ARDUINO_USB_CDC_ON_BOOT=0`) are
> intentionally absent — they are only needed when GPIO19/20 are actually wired for
> native USB HID. Without them, USB-C serial (CP2102) works normally for flashing/debug.

---

## Architecture

```
  Phone browser ──WiFi (HTTP + WebSocket)──▶ ESP32-S3
                                               │
        ┌──────────────────────────────────────┤
        ▼                                       ▼
   Web server  ──▶ input queue ──▶  Tamagotchi core (FSM)
   (LittleFS                          │        │
    static UI)                        │        ├─▶ E-ink renderer ─▶ panel
                                       ▼        │
                              HID dispatcher    └─ stats: hunger, mood, energy…
                               └─▶ BLE HID  (NimBLE) ─▶ phone/PC
```

**States:** `IDLE`, `SLEEP`, `EAT`, `PLAY`, `KEYBOARD`.

Design hook: being a keyboard drains energy / increases hunger, so HID usage is a care
mechanic — the pet works as your keyboard, then needs feeding.

## Project layout

```
project-tama/
├── platformio.ini
├── CLAUDE.md
├── README.md
├── bin/
│   ├── pio          # wrapper → pioarduino penv (see Toolchain section)
│   └── flash        # two-step firmware+FS uploader
├── data/
│   └── index.html   # companion web UI (WebSocket, touchpad + key grid)
└── src/
    ├── main.cpp           # setup/loop, uiTask, side-effect runner
    ├── Input.h            # InputEvent struct (KEY/MOVE/CLICK)
    ├── AppContext.h       # bundle: fsm/ble/slots/ctrl/nav/led refs handed to pages
    ├── app/
    │   └── Controller.cpp/.h   # single command sink; pages + WS both go through it
    ├── pet/
    │   ├── PetState.h      # State enum + Pet struct (hunger, mood, energy)
    │   └── PetFSM.cpp/.h   # PURE: only tick/transitions, depends on HidBackend
    ├── hid/
    │   ├── HidBackend.h    # abstract interface (begin/end/key/mouseMove/mouseClick)
    │   ├── UsbHid.cpp/.h   # stub (GPIO19/20 not wired)
    │   └── BleHid.cpp/.h   # NimBLE HID-over-GATT + 3-slot host switcher
    ├── io/
    │   ├── Button.cpp/.h       # ISR → queue → debounce task (Core 0, prio 5, 8 KB)
    │   ├── Battery.cpp/.h      # ADC % (GPIO20 via GPIO19 gate)
    │   ├── Clock.cpp/.h        # HH:MM (companion-synced; uptime fallback)
    │   ├── DeviceSlots.cpp/.h  # 3 BLE host slots, persisted in NVS ("slots" ns)
    │   ├── Led.cpp/.h          # onboard LED blinker (GPIO18, Routine subclass)
    │   ├── PetStore.cpp/.h     # pet stat persistence in NVS ("pet" ns)
    │   └── Settings.cpp/.h     # user preferences in NVS ("settings" ns)
    ├── net/
    │   ├── DebugPage.cpp/.h        # /debug HTML builder
    │   ├── WebUI.cpp/.h            # HTTP server + WS dispatch → Controller
    │   └── StateBroadcaster.cpp/.h # JSON push to all WS clients
    ├── routine/
    │   ├── Routine.h               # abstract base: tick() + intervalMs()
    │   ├── RoutineManager.h/.cpp   # calls each routine at its interval from loop()
    │   └── BatteryRoutine.h/.cpp   # 60 s: Battery::sample() + pushState()
    └── ui/
        ├── Canvas.h              # mono drawing surface interface
        ├── Display.h
        ├── EinkDisplay.cpp/.h    # heltec-eink-modules backend
        ├── Header.cpp/.h         # 10px header: custom text | clock, battery %, icons
        ├── ListView.cpp/.h       # vertical list, selection = inversion-highlight
        ├── Icons.cpp/.h          # 8x8 XBM glyphs (BT, lock, heart…)
        ├── Page.h
        ├── PageStack.cpp/.h      # dual-core carousel nav (Core 0 nav / Core 1 render)
        └── pages/
            ├── HomePage.cpp/.h         # pet face + stat bars
            ├── CarePage.cpp/.h         # Feed / Play / Sleep (modal, owned by HomePage)
            ├── KeyboardPage.cpp/.h     # 3 host slots + BLE toggle + Back
            ├── SlotActionPage.cpp/.h   # modal: Pair / Connect / Forget per slot
            ├── PairingPage.cpp/.h      # modal: 6-digit BT PIN display
            └── SettingsPage.cpp/.h     # airplane mode + LED enable
```

## Interaction model (device-native)

The board is fully usable on its own through one button:

- **Short press at root**: cycle to the next root page (Home → Keyboard → Settings).
- **Long press at root on Home**: opens the Care modal (Feed / Play / Sleep) directly.
  Selecting an action dismisses the modal and returns to Home.
- **Long press at root on Keyboard / Settings**: enters the page. The header inverts
  (black bg, white text) to signal "inside a page".
- **Short press inside a page**: advance the page's list selection.
- **Long press inside a page**: execute the selected action (or close a modal).
- **Long press on Back item**: leaves to root carousel (header returns to normal).
- **Screen auto-pauses** after 30 s of inactivity — first button press unpauses without
  navigating. Header shows `PAUSED | HH:MM` while paused.
- Reset is hardware-only.

`CarePage` is **not** a root page — it is owned by `HomePage` as a private member and
pushed as a modal via `HomePage::interceptsRootLongPress()`. Any page can override
`Page::interceptsRootLongPress(PageStack&)` (returns `false` by default) to capture
the carousel long-press without entering the page.

The web UI at `http://tamaboard.local/` (or `http://192.168.4.1/` as fallback) is a
**companion**: it mirrors pet stats via WebSocket, lets you manage BLE host slots
remotely, and provides the keyboard input surface (touchpad + key grid) used while the
device is in `KEYBOARD` state. mDNS hostname `tamaboard` is registered via `ESPmDNS`
(built into the ESP32 Arduino core — no extra lib dep) and re-registered after
wake-from-sleep. Works natively on macOS, iOS, Linux; Windows 10+ also resolves `.local`.

A hidden debug page is available at `http://tamaboard.local/debug` — uptime, heap,
battery, pet stats, BLE status, reset reason. Not linked from the main UI.

## WebSocket protocol — the only API

The board is the **single source of truth**. The companion subscribes via a
WebSocket at `ws://<board-ip>/ws` and receives push messages on every state
change. There is no REST and no polling.

### Client → server (commands, JSON text frames)

| `t`     | fields                                                            | meaning                            |
|---------|-------------------------------------------------------------------|------------------------------------|
| action  | `state`: `EAT\|PLAY\|SLEEP\|IDLE\|KEYBOARD`                       | FSM transition                     |
| slot    | `i`: `0..2`, `action`: `select\|pair\|forget`                     | BLE host slot operation            |
| clock   | `local_seconds`: `0..86399`                                       | companion's local seconds-of-day   |
| key     | `code`: HID keycode, `down`: bool                                 | HID keyboard (KEYBOARD state only) |
| move    | `dx`, `dy`: int8                                                  | HID mouse relative move            |
| click   | `btn`: `1\|2\|4`                                                  | HID mouse click                    |

### Server → client (pushed on every change + on connect)

| `t`     | fields                                                                                                           |
|---------|------------------------------------------------------------------------------------------------------------------|
| state   | `state, hunger, mood, energy, battery, ble, ble_slot, ble_slot_name, clock_synced`                               |
| slots   | `active, slots:[{name, bonded, connected}×3]`                                                                    |
| pairing | `passkey`: 6-digit int (0 = dismiss)                                                                             |

`ble` is one of `IDLE\|ADVERTISING\|PAIRING\|CONNECTED\|FAILED`.

## Architecture (detailed)

```
                ┌────── PageStack (Core 1 — UI task) ──────┐
                │                                           │
                ▼                                           ▼
            Pages (Home, Care, KB, Settings)    ──reads── PetFSM, BleHid, slots
                │
                ▼
            ┌─────────────┐                       ┌──────────────────┐
   WS ─────▶│ Controller  │── mutates state ────▶ │ PetFSM, BleHid,  │
            └─────────────┘                       │ DeviceSlots,Clock│
                ▲                                  └──────────────────┘
                │                                            │
                │                                  on change ▼
                │                                  ┌──────────────────┐
                │                                  │ State callbacks  │
                │                                  └──────────────────┘
                │                                            │
                │                                            ▼
                │                                  ┌──────────────────┐
                │                                  │ StateBroadcaster │── WS push to all clients
                │                                  └──────────────────┘
                │
            ┌────────┐
            │ WebUI  │  (HTTP + WS plumbing; dispatches incoming WS → Controller)
            └────────┘
```

### Task layout

| Task | Core | Priority | Description |
|------|------|----------|-------------|
| `loopTask` (Arduino `loop()`) | 0 | 1 | FSM tick, routines, auto-pause check |
| `buttonTask` | 0 | 5 | ISR → queue → debounce → nav callbacks |
| `uiTask` | 1 | 3 | Blocks on semaphore, renders when dirty |
| AsyncWebServer / NimBLE | 0 | internal | Network I/O |

### SOLID alignment
- **SRP** — each module owns one thing: `PetFSM` (pet stats + transitions only),
  `Controller` (verbs), `StateBroadcaster` (JSON push), `WebUI` (server+WS plumbing),
  `EinkDisplay` (panel I/O), pages (one screen each).
- **OCP** — adding a new page = subclass `Page`. New display = implement `Display`+
  `Canvas`. New NVS domain = new `XxxStore` module. New broadcast field = touch one
  builder in `StateBroadcaster`. New background task = subclass `Routine`.
- **LSP** — `BleHid`/`UsbHid` both honor `HidBackend`; `EinkDisplay` honors `Display`.
- **ISP** — `HidBackend` is intentionally minimal (no slot ops; those live on
  `BleHid` only, since "switch host" has no meaning for USB).
- **DIP** — `PetFSM` depends on `HidBackend&`, not concrete `BleHid`. Pages
  depend on `Controller`/`PageStack` interfaces, not on transports.

### Side-effect runner
`PetFSM` is pure state. `main.cpp::onFsmStateChange` is the single subscriber that
performs the dirty work of state changes: HID lifecycle on `KEYBOARD`±, WiFi+BLE
off / `esp_light_sleep_start()` on `SLEEP`, and `transition(IDLE)` on wake.
This keeps the FSM testable and the side effects discoverable in one place.

---

## Flashing — read this first

**Connect the LiPo battery before flashing, not just USB-C.** The Wireless Paper's
regulator can't reliably power the ESP32-S3 through download-mode current spikes
on USB-C alone — symptom is esptool getting "Download mode successfully detected"
followed by "no sync reply / TX path seems to be down" and timing out.
With the battery attached the rail is stable and flashing works normally.

**Use the `bin/flash` wrapper, not `pio run -t upload -t uploadfs`.**
Pioarduino disables LDF (library dep resolution) whenever any filesystem target
is on the command line, so chaining `uploadfs` with `upload` makes the firmware
compile fail with cryptic "Header.h: No such file or directory" errors from
core libraries. The wrapper runs them as two separate `pio run` invocations:

```
bin/flash             # firmware only
bin/flash --fs        # firmware + web UI (data/)
bin/flash --fs-only   # web UI only
```

If you ever flash *over* something aggressive (e.g. Meshtastic), the existing
firmware can grab the UART before esptool finishes syncing. Workaround: enter
download mode manually (hold **PRG**, tap **RST**, release **PRG**) and run
`bin/pio run -t erase` before flashing.

---

## Hardware-specific pin notes

- **Onboard LED** on `GPIO18` (active-HIGH, `LED_BUILTIN`). Driven by the `Led` routine.
  Confirmed by heltec-eink-modules platform header and Meshtastic's variant.h.
- **Battery sensing** (matches Meshtastic's Wireless Paper variant):
  - `BATTERY_PIN = GPIO20` (ADC2 channel, ~1:1 divider)
  - `ADC_CTRL = GPIO19` (active-**low** gate — pull LOW to enable divider, HIGH to disable; P-FET)
  - ADC2 reads can be unreliable while WiFi/BT radios are active on ESP32-S3.
    Sample infrequently (`BatteryRoutine` fires every 60 s).
- **PRG button** on `GPIO0` (active-low, internal pull-up). Configured as ext0 wake
  source so a press wakes the board from light sleep in the SLEEP/lock state.
- **Vext** on `GPIO45` (active-low). heltec-eink-modules manages this internally.
- **SX1262 LoRa**: never initialized — left in reset. Saves power.

## Display abstraction

`Canvas` is a tiny mono-drawing interface. Pages render through it without knowing
the backing hardware. `EinkDisplay` is the only current backend; adding an OLED or
larger e-ink later means writing a new `Display` + `Canvas` adapter — pages don't
change.

`EinkCanvas` tracks an `_inverted` bool: `setInverted(true)` flips all subsequent
`fillRect`, `drawBitmap`, and `hline` calls (white-on-black). Used by `Header` to
render the fully inverted header bar when the user is inside a page.

## Header

The 10 px header bar shows:
- **Left:** page title (or inverted when inside a page), lock icon on SLEEP.
- **Center:** `TEXT | HH:MM` when `headerSetCustomText("TEXT")` is set (e.g. `"SLEEP"`,
  `"PAUSED"`); otherwise just `HH:MM`.
- **Right:** battery percent (`%u%%`) + airplane/BT icons.

`headerSetCustomText(nullptr)` clears the override and restores the clock-only display.

## Routine framework

Background tasks that need periodic ticking implement `Routine`:

```cpp
class Routine {
public:
    virtual void begin() {}
    virtual void tick() = 0;
    virtual uint32_t intervalMs() const = 0;
};
```

`RoutineManager::add(Routine*)` calls `begin()` once, then `tick()` at `intervalMs()`
cadence from `loop()`. Current routines:

| Routine | Interval | Does |
|---------|----------|------|
| `Led` | 20 ms | step-sequence LED patterns (SINGLE/DOUBLE/TRIPLE/FAST) |
| `BatteryRoutine` | 60 s | `Battery::sample()` + `StateBroadcaster::pushState()` |

`Led` extends `Routine` rather than using a FreeRTOS timer — it shares the loop task
and keeps LED timing in user-space without adding a task.

## NVS persistence convention

Each module that needs persistence owns its own Preferences namespace:

| Module         | Namespace    | Keys                                              |
|----------------|--------------|---------------------------------------------------|
| `DeviceSlots`  | `"slots"`    | `active`, `occ{i}`, `name{i}`, `addr{i}`         |
| `PetStore`     | `"pet"`      | `hunger`, `mood`, `energy`, `state`               |
| `Settings`     | `"settings"` | `led` (bool, default true), `airplane` (bool, default false) |

Adding new keys to an existing namespace requires no migration — missing NVS keys
return the typed default. Adding a new module: create `src/io/XxxStore.h/.cpp`,
claim a new namespace (≤15 chars), follow the same `begin()` / `save()` lifecycle.

`PetFSM` stays pure — `PetStore::begin()` / `PetStore::save()` are called from
`main.cpp`. Pet state is persisted on SLEEP entry and every 60 ticks (~5 min).
Only `IDLE` and `SLEEP` survive reboot; `EAT`/`PLAY`/`KEYBOARD` reset to `IDLE`.

## Power notes

- **WiFi TX power** is set to 8.5 dBm (`WIFI_POWER_8_5dBm`) — the default 20 dBm
  is excessive for phone-width range and causes noticeable chip warmth. Applied after
  both the initial `softAP()` in setup and the wake-from-sleep WiFi restore.
- **SLEEP state** drops both WiFi and BLE before entering `esp_light_sleep_start()`.
  On wake, WiFi AP is restored; BLE only resumes when the user re-enters KEYBOARD.
- **SX1262 LoRa** is never initialized. **ADC2** sampling is guarded to run only when
  neither WiFi nor BLE is in use (or infrequently enough to tolerate noise).

## Key abstraction: HID backends behind one interface

```cpp
struct HidBackend {
  virtual void begin() = 0;
  virtual void end()   = 0;
  virtual void key(uint8_t keycode, bool down) = 0;
  virtual void mouseMove(int8_t dx, int8_t dy)  = 0;
  virtual void mouseClick(uint8_t button)       = 0;
};
// UsbHid : HidBackend  → wraps USBHIDKeyboard / USBHIDMouse (stubbed)
// BleHid : HidBackend  → wraps a NimBLE HID-over-GATT service
```

`onFsmStateChange` in `main.cpp` calls `bleHid.begin()` on entering `KEYBOARD` and
`bleHid.end()` on leaving it. `PetFSM` holds a `HidBackend&` but never calls it
directly — the side-effect runner does.

---

## Display constraints

Mono 250×122 px (portrait, `setRotation(3)`). Full refresh ~1s with flash; partial
refresh faster but ghosts — `MAX_PARTIALS = 100` forces a full refresh every ~100
partial updates. Design around discrete states and event-driven updates, not animation.
Mono framebuffer ≈ 3.8 KB.

UI scale: `UI_SCALE = 2` (Adafruit-GFX built-in 5×7 font at 2×). `lineHeight()` = 16.
Scale-1 text (`setTextSize(1)`) is 6px wide × 8px tall — used for footers and headers.
Header height is 10px. `setInverted(true)` is Canvas-level, not display-level.

## PageStack threading model

`PageStack` is the dual-core boundary:

- **Core 0** (button task, loop task) calls nav methods (`onShortPress`, `onLongPress`,
  `leave`, `goHome`, `pushModal`, `popModal`) and `requestRender()`.
- **Core 1** (`uiTask`) runs `renderIfDirty()` in a tight loop, blocking on a FreeRTOS
  binary semaphore (`_renderSem`).

Synchronization:

| Resource | Mechanism |
|----------|-----------|
| `_dirty` | `std::atomic<bool>` (release/acquire) |
| Nav state (`_rootIdx`, `_entered`, `_modal`) | `portMUX_TYPE _navMux` — snapshot before render, locked around mutations |
| Render signal | FreeRTOS binary semaphore — given by `requestRender()`, taken by uiTask |
| `_screenPaused`, `_suppressNextInput` | `volatile bool` — Core 0 writes, Core 1 reads |
| `_lastRenderMs` | `volatile uint32_t` — Core 1 writes, Core 0 reads; one-cycle stale is fine |

Sleep-entry render bypasses the semaphore: `vTaskSuspend(uiTask)` →
`renderDirectly()` (Core 0) → sleep → wake → `vTaskResume(uiTask)`.

## Toolchain — important

**Use `./bin/pio`, not Homebrew's `pio`.** The pioarduino fork of
platform-espressif32 ships build-script C extensions (`littlefs-python`, `fatfs`)
compiled for the **CPython 3.11 ABI** — they don't run under Homebrew's Python 3.14
pio. The `./bin/pio` wrapper invokes the pioarduino-installed PlatformIO
(`~/.platformio/penv/bin/pio`, Python 3.11), which has the matched extensions.

If `~/.platformio/penv/` doesn't exist yet, install pioarduino's PlatformIO:
```
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/pioarduino/pioarduino-core-installer/pioarduino/get-platformio.py
python3 get-platformio.py
```
Then `./bin/pio run` to build, `./bin/pio run -t upload` to flash, `./bin/pio run -t uploadfs` to upload the `data/` web UI.

## Current deviations from original design

- **USB HID is stubbed.** `UsbHid` is a no-op. The USB build flags are absent from
  `platformio.ini` — add them only when GPIO19/20 are physically wired.
- **Single `KEYBOARD` state.** BLE HID via NimBLE is the only active backend.
- **BLE pairing shows a 6-digit PIN on the e-ink** (DisplayOnly/MITM). PIN is
  generated with `esp_random()` in `onPassKeyDisplay()`.
- **3-slot device switcher.** Up to 3 hosts can be bonded; the keyboard page lets
  you switch and shows a context submenu (Pair / Connect / Forget) per slot via
  `SlotActionPage` modal. Slot metadata persists in NVS.
- **Pet stats persist across reboots** via `PetStore` (NVS namespace `"pet"`).
- **SLEEP = lockscreen:** drops WiFi + BLE, light-sleeps CPU, wakes on PRG (GPIO0).
  `pageStack.goHome()` is called before sleeping so the e-ink always shows the clean
  sleep face.
- **WiFi TX power capped at 8.5 dBm** to reduce chip temperature during idle.
- **mDNS hostname `tamaboard.local`** registered via `ESPmDNS` after `softAP()` in setup
  and after the wake-from-sleep WiFi restore. Fallback IP `192.168.4.1` unchanged.
- **Settings page** has three items: airplane mode toggle (disables WiFi + BLE on next
  boot), LED enable/disable toggle, Back. Both settings persist in NVS (`"settings"`).
- **Onboard LED (GPIO18)** blinks on state changes: 1× generic, 2× BLE connected,
  3× BLE failed, rapid during pairing. Implemented as a `Routine` subclass with a
  20 ms step-sequence tick. Can be disabled from Settings.
- **Battery shown as percent text** (`%u%%`) right-aligned in the header — not a
  rectangle widget.
- **Screen auto-pause** after 30 s idle — display locks, header shows `PAUSED | HH:MM`,
  first button press unpauses without navigating.
- **Dual-core rendering** — `uiTask` on Core 1 owns all e-ink I/O; nav runs on Core 0.
  Core 0 never blocks on display operations.

## Notes / conventions

- Dev host: macOS, fish shell, tmux.
- Flash/monitor: `bin/flash` / `bin/pio device monitor`.
- Keep `CLAUDE.md` lean; move long-form narrative to `docs/` and reference on demand.
- TinyUSB HID and NimBLE BLE HID coexist with WiFi fine for keystroke/mouse traffic
  (light load). Memory is the real budget — keep the web bundle small.
- The tick interval is `TICK_INTERVAL_MS = 5000` ms. Home re-render fires every 5 s
  via `Page::autoRefreshMs()` override. The constant lives at the top of `main.cpp`.
- The companion web UI auto-syncs the clock on every WebSocket connect (`syncClock()`
  in `ws.onopen`) — no NTP needed.
