# 🖥️ Cyberdeck — Arduino PC Dashboard

A DIY **always-on desktop companion** powered by an **Arduino Uno** and a **16×2 I²C LCD**. It displays live PC stats, real-time Spotify playback with a smooth progress bar, time-synced scrolling lyrics, and a flame-detection alarm — all controlled from four tactile buttons with short/long press logic.

---

## 📸 Feature Overview

| Screen | What you see |
|---|---|
| **Mode 0 — PC Stats** | CPU %, RAM %, CPU temperature, GPU temperature |
| **Mode 1 — Spotify** | Scrolling song/artist name + 80-segment sub-pixel progress bar |
| **Mode 2 — Lyrics** | Typewriter-animated, time-synced LRC lyrics |
| **🔥 Flame Alert** | Instant audio alarm via Windows native media player |

---

## 🗂️ Project Structure

```
Arduino Project/
├── sketch_jun3a/
│   └── sketch_jun3a.ino      # Arduino firmware (C++)
├── cyberdeck_win.py           # Windows host companion script (Python)
├── MKBAAG.mp3                 # Alarm audio played on flame detection
├── test_media.py              # Utility: test Windows media session API
├── test_temp.py               # Utility: test WMI temperature readings
├── test_hindi.py              # Utility: test Unicode/Unidecode conversion
├── test_lrc.py                # Utility: test LRC lyrics parser
├── test_lrc_search.py         # Utility: test LRClib API search
├── test_lyrics_debug.py       # Utility: debug lyrics sync timing
├── test_nvml.py               # Utility: test NVIDIA NVML GPU library
├── test_pb.py                 # Utility: test progress bar rendering
├── test_speed.py              # Utility: test serial throughput
└── test_timeline.py           # Utility: test Windows timeline/position API
```

---

## 🔩 Hardware Components

| Component | Description |
|---|---|
| **Arduino Uno (or Nano)** | The microcontroller brain |
| **16×2 I²C LCD** | Display module (PCF8574 backpack, address `0x27` or `0x3F`) |
| **4× Tactile Push Buttons** | Navigation and media control |
| **1× Flame / IR Sensor Module** | Digital fire/flame detector (active LOW) |
| **USB Cable** | Powers the Arduino and provides serial communication to PC |
| **Breadboard + Jumper Wires** | For prototyping connections |

---

## 🔌 Wiring & Pin Connections

### I²C LCD (16×2 with PCF8574 backpack)

| LCD Pin | Arduino Pin | Notes |
|---|---|---|
| `VCC` | `5V` | Power |
| `GND` | `GND` | Ground |
| `SDA` | `A4` | I²C Data |
| `SCL` | `A5` | I²C Clock |

> The I²C backpack communicates over just two wires (SDA + SCL), freeing up most digital pins.
> Default I²C address is **`0x27`**. If the display is blank, try **`0x3F`** (change in the `.ino` file, line 5).

---

### Push Buttons (active LOW with internal pull-ups)

All buttons are wired between their respective Arduino digital pin and **GND**. The Arduino's internal `INPUT_PULLUP` resistor holds the line HIGH when the button is open, and the pin reads LOW when the button is pressed.

| Button | Arduino Pin | Short Press Action | Long Press Action |
|---|---|---|---|
| **BTN_A** | `D2` | Cycle display mode (0→1→2→0) | Send `PLAY_PAUSE` to PC |
| **BTN_B** | `D8` | Send `NEXT` track to PC | Send `PREV` track to PC |
| **BTN_C** | `D6` | Send `BTN_C_SHORT` (extensible) | Send `BTN_C_LONG` (extensible) |
| **BTN_D** | `D5` | Send `BTN_D_SHORT` (extensible) | Send `BTN_D_LONG` (extensible) |

**Wiring for each button:**
```
Arduino Pin [D2 / D8 / D6 / D5]
         │
        [Button]
         │
        GND
```

> No external resistors required — the Arduino's built-in `INPUT_PULLUP` handles this.

---

### Flame / IR Sensor Module

| Sensor Pin | Arduino Pin | Notes |
|---|---|---|
| `VCC` | `5V` | Power |
| `GND` | `GND` | Ground |
| `DO` (Digital Out) | `D7` | Reads LOW when flame detected |

> Most flame sensor modules have a built-in potentiometer to adjust sensitivity.
> The sensor outputs **LOW** when a flame/IR source is detected and **HIGH** at idle.

---

### Complete Wiring Diagram (ASCII)

```
                        Arduino Uno
                    ┌──────────────────┐
              5V ───┤ 5V          D2   ├─── [BTN_A] ─── GND
             GND ───┤ GND         D8   ├─── [BTN_B] ─── GND
              A4 ───┤ SDA (A4)   D6   ├─── [BTN_C] ─── GND
              A5 ───┤ SCL (A5)   D5   ├─── [BTN_D] ─── GND
                    │            D7   ├─── Flame Sensor DO
                    │            USB  ├═══ PC (Serial + Power)
                    └──────────────────┘
                         │    │
                        SDA  SCL
                         │    │
                    ┌──────────────────┐
                    │  16×2 I²C LCD    │
                    │  (PCF8574 0x27)  │
                    └──────────────────┘
             VCC / GND ───┤ 5V / GND
```

---

## 💻 How the System Works

The project has two halves that communicate over **USB serial at 9600 baud**:

```
┌─────────────────────────────┐            ┌──────────────────────────────────┐
│         PC (Python)         │            │       Arduino (C++ Firmware)     │
│                             │            │                                  │
│  ┌─────────────────────┐    │  Serial    │  ┌────────────────────────────┐  │
│  │ System Stats        │────┼──────────►│  │ Parse data packet          │  │
│  │ (CPU, RAM, Temps)   │    │            │  │ Update LCD display         │  │
│  └─────────────────────┘    │            │  └────────────────────────────┘  │
│                             │            │                                  │
│  ┌─────────────────────┐    │  Serial    │  ┌────────────────────────────┐  │
│  │ Spotify / Media     │────┼──────────►│  │ Scroll song name           │  │
│  │ (Title, Progress)   │    │            │  │ Draw progress bar          │  │
│  └─────────────────────┘    │            │  └────────────────────────────┘  │
│                             │            │                                  │
│  ┌─────────────────────┐    │  Serial    │  ┌────────────────────────────┐  │
│  │ LRClib Lyrics API   │────┼──────────►│  │ Typewriter-animate lyric   │  │
│  │ (Synced LRC)        │    │            │  └────────────────────────────┘  │
│  └─────────────────────┘    │            │                                  │
│                             │◄───────────┼─ Button presses / Flame alerts  │
└─────────────────────────────┘            └──────────────────────────────────┘
```

---

## 📡 Serial Communication Protocol

### PC → Arduino (data packets)

**System + Spotify packet** (sent every ~1 second):
```
<CPU:42% RAM:61%|C:52C G:68C|Artist - Song Title|73>
 ──────────────   ────────────  ─────────────────   ──
    sysLine1        sysLine2       spotifyText      progressPct
```
- Wrapped in `< ... >` angle brackets
- Fields separated by `|` pipes
- `progressPct` is an integer 0–100

**Lyric packet** (sent on every lyric change, up to 10Hz):
```
<L|This is the current lyric line>
```
- Always prefixed `<L|` so the Arduino can distinguish it from a stats packet

---

### Arduino → PC (commands)

| Command | Trigger |
|---|---|
| `PLAY_PAUSE` | BTN_A long press |
| `NEXT` | BTN_B short press |
| `PREV` | BTN_B long press |
| `FLAME_DETECTED` | Flame sensor goes LOW |
| `BTN_C_SHORT` | BTN_C short press |
| `BTN_C_LONG` | BTN_C long press |
| `BTN_D_SHORT` | BTN_D short press |
| `BTN_D_LONG` | BTN_D long press |

---

## 🖥️ Arduino Firmware Deep Dive (`sketch_jun3a.ino`)

### Initialization (`setup()`)
1. Opens serial at **9600 baud**
2. Sets all button pins to `INPUT_PULLUP`
3. Sets flame sensor pin to `INPUT`
4. Initializes the LCD and turns on the backlight
5. Defines **5 custom sub-pixel characters** (used for the smooth progress bar)
6. Prints `"CYBERDECK INIT"` splash screen for 1.5 seconds

### Main Loop (`loop()`)
Each iteration of the loop calls four functions in order:
```
loop()
  ├── readSerialData()    → parse incoming packets from PC
  ├── handleButtons()     → check all 4 buttons for press events
  ├── checkFlameSensor()  → poll flame sensor, emit alert if triggered
  └── updateDisplay()     → render the correct screen to LCD
```

### Button Debounce & Long-Press Detection
The `checkButton()` function implements a state machine per button:

```
Button released → wait for LOW edge → record pressTime
                                           ↓
Button held LOW → if held ≥ 500ms → fire onLongPress(), set isPressed=false
                                           ↓
Button released → if held 50–500ms → fire onShortPress()
```

- **`DEBOUNCE_MS = 50`** — ignores noise/bounces shorter than 50ms
- **`LONG_PRESS_MS = 500`** — holds longer than 500ms = long press
- Long press fires **once** (not repeatedly) while held

### Display Mode 0 — PC Stats
- Prints `sysLine1` (CPU + RAM) on the top row
- Prints `sysLine2` (CPU temp + GPU temp) on the bottom row
- Both lines are updated every 1 second from the PC

### Display Mode 1 — Spotify
**Top row — Scrolling song name:**
- If title ≤ 16 characters: print statically with padding
- If title > 16 characters: circular scroll (`scrollPos` increments every 400ms), wraps with 3-space gap between repeats

**Bottom row — Sub-pixel progress bar:**
- 16 LCD characters × 5 sub-pixel columns = **80 total segments**
- `totalBlocks = (progressPct × 80) / 100`
- `fullBlocks = totalBlocks / 5` full-block characters (custom char 5: `█`)
- `partialBlock = totalBlocks % 5` selects one of 5 custom fractional characters (chars 1–5)
- Creates a smooth analog-style progress bar despite the discrete character grid

### Display Mode 2 — Lyrics (Typewriter)
- Receives the current lyric string via `<L|...>` packets from the PC
- Renders characters one at a time at 25ms per character (40 chars/sec)
- Wraps across both LCD rows (32 characters total visible area)
- Auto-paginates: once 32 characters are typed, the next 32-character block starts
- Resets animation whenever a new lyric arrives

### Flame Sensor
- Polls `D7` every loop iteration
- On falling edge (HIGH → LOW): sets `flameDetected = true`, sends `FLAME_DETECTED` over serial
- On rising edge (LOW → HIGH): clears `flameDetected`
- Edge detection prevents flooding the serial port with repeated alerts

---

## 🐍 Python Host Script Deep Dive (`cyberdeck_win.py`)

### Dependencies

| Library | Purpose |
|---|---|
| `pyserial` | Serial communication with Arduino |
| `psutil` | CPU %, RAM % |
| `pynvml` | NVIDIA GPU temperature via NVML |
| `wmi` | CPU temperature via WMI (`MSAcpi_ThermalZoneTemperature`) |
| `winrt` | Windows Runtime — reads current media session (Spotify, browser, etc.) |
| `pyautogui` | Sends media key presses (`play/pause`, `next`, `prev`) |
| `unidecode` | Converts Unicode/Hindi/emoji text to ASCII for LCD compatibility |
| `urllib` | HTTP requests to LRClib API (no extra dependency) |
| `threading` | Background lyrics fetching (non-blocking) |
| `asyncio` | Async event loop for WinRT media API |
| `subprocess` | Launches PowerShell to play alarm MP3 |

### Async Main Loop
The script runs two nested update loops inside a single `asyncio` event loop:

#### Fast Loop — 10Hz (`> 0.1s` interval)
- Queries the **Windows media timeline** to get playback position (`pos`) and duration
- Calculates `progress` percentage
- Scans `lyrics_data` to find the current lyric using a **0.4-second lookahead** (so the typewriter animation finishes right as the singer starts the line)
- If the lyric changed, encodes it with `unidecode` and sends `<L|...>\n` to Arduino

#### Slow Loop — 1Hz (`> 1.0s` interval)
- Reads **CPU %** and **RAM %** via `psutil`
- Reads **CPU temperature** via WMI (requires admin/ACPI access)
- Reads **GPU temperature** via `pynvml` (NVIDIA only)
- Queries the **Windows Global System Media Transport Controls** for song title and artist
- If the song changed, launches a **background thread** to fetch synced lyrics from LRClib
- Assembles and sends the full stats+media packet: `<sysLine1|sysLine2|song|progress>\n`

### Lyrics System
1. On song change → `threading.Thread(target=fetch_lyrics_bg, ...)` starts
2. `fetch_lyrics_bg()` queries `https://lrclib.net/api/search?q=<title+artist>`
3. Picks the first result that contains `syncedLyrics` (LRC format)
4. `parse_lrc()` parses `[mm:ss.xx]` timestamps into a sorted list of `(seconds, text)` tuples
5. The fast loop scans this list every 100ms to find and send the correct line

### Flame Alert
When `FLAME_DETECTED` is received from Arduino:
- Builds the absolute path to `MKBAAG.mp3` (co-located with the script)
- Launches a **hidden PowerShell process** using Windows `System.Windows.Media.MediaPlayer`
- No extra audio library (pygame, playsound, etc.) needed — uses Windows native stack

### Command Handler
Runs every loop iteration (non-blocking, checks `arduino.in_waiting`):
- `PLAY_PAUSE` → `pyautogui.press('playpause')`
- `NEXT` → `pyautogui.press('nexttrack')`
- `PREV` → `pyautogui.press('prevtrack')`
- `FLAME_DETECTED` → plays MP3 alarm via PowerShell
- `BTN_C_*` / `BTN_D_*` → printed to console (user-extendable)

---

## 🚀 Setup & Installation

### 1. Arduino Side

1. Install the **LiquidCrystal_I2C** library in the Arduino IDE:
   - Sketch → Include Library → Manage Libraries → search `LiquidCrystal I2C` (by Frank de Brabander)
2. Open `sketch_jun3a/sketch_jun3a.ino` in the Arduino IDE
3. Select your board (**Tools → Board → Arduino Uno**) and the correct port
4. Upload the sketch
5. Open Serial Monitor to verify `CYBERDECK INIT` appears (set baud to 9600)
6. **Close the Serial Monitor** before running the Python script (only one process can own the serial port)

### 2. Python Side

```bash
pip install pyserial psutil pynvml wmi pyautogui winrt-runtime unidecode
```

> **Note:** `winrt-runtime` provides `winrt.windows.media.control`. Install it, not the older `winsdk`.

### 3. Configuration

Edit the top of `cyberdeck_win.py` if needed:

```python
SERIAL_PORT = 'COM3'   # ← Change to your Arduino's COM port
BAUD_RATE = 9600       # ← Must match the .ino file
```

Find your COM port: Device Manager → Ports (COM & LPT) → "USB-SERIAL CH340" or "Arduino Uno".

### 4. Run

```bash
python cyberdeck_win.py
```

> **Run as Administrator** if CPU temperature always shows `--` (WMI ACPI access requires elevated privileges).

---

## ⚠️ Troubleshooting

| Problem | Solution |
|---|---|
| LCD is blank / backlight only | Try I²C address `0x3F` in line 5 of `.ino` |
| `Failed to connect to COM3` | Close Arduino IDE Serial Monitor; check Device Manager for correct port |
| CPU temp shows `--` | Run Python script as Administrator |
| GPU temp shows `--` | Only works with NVIDIA GPUs via NVML; AMD/Intel not supported |
| Lyrics never appear | Song must be playing through a system-level media session (Spotify app, not browser) |
| Non-ASCII lyrics garbled | `unidecode` converts Unicode to closest ASCII; install with `pip install unidecode` |
| Flame alarm won't play | Ensure `MKBAAG.mp3` is in the same directory as `cyberdeck_win.py` |

---

## 🔧 Utility / Debug Scripts

These helper scripts can be run independently to test individual subsystems:

| Script | What it tests |
|---|---|
| `test_media.py` | Prints the current song title & artist from the Windows media session |
| `test_temp.py` | Tests WMI ACPI temperature readings and PowerShell CIM fallback |
| `test_nvml.py` | Checks NVIDIA NVML initialization and GPU temperature |
| `test_lrc.py` | Tests LRC timestamp parsing |
| `test_lrc_search.py` | Tests LRClib API search query |
| `test_lyrics_debug.py` | Debugs lyrics sync timing and lookahead logic |
| `test_hindi.py` | Verifies Unidecode handles non-Latin scripts |
| `test_pb.py` | Tests progress bar sub-pixel character rendering |
| `test_speed.py` | Measures serial write throughput |
| `test_timeline.py` | Tests Windows media timeline position/duration properties |

---

## 🧩 Extending the Project

### Adding a custom button action
In `cyberdeck_win.py`, find the `handle_commands()` function and add:
```python
elif cmd == "BTN_C_SHORT":
    # your custom action here, e.g. open a URL, toggle an app, etc.
    os.startfile("https://example.com")
```

### Changing the scroll speed
In `sketch_jun3a.ino`, change the value in:
```cpp
if (millis() - lastScroll > 400) { // ← milliseconds per scroll step
```

### Changing the lyric typewriter speed
```cpp
if (millis() - lastTypeTime > 25) { // ← milliseconds per character
```

### Changing the long-press threshold
```cpp
const int LONG_PRESS_MS = 500; // ← increase for longer hold required
```

### Supporting AMD GPU temperatures
Replace the `get_temp_gpu()` function with `wmi.WMI().Win32_VideoController()` and add appropriate parsing for AMD Radeon sensors.

---

## 📦 Dependencies Summary

```
Arduino:
  - LiquidCrystal_I2C (by Frank de Brabander)
  - Wire (built-in)

Python:
  - pyserial
  - psutil
  - pynvml
  - wmi
  - pyautogui
  - winrt-runtime
  - unidecode
```

---

## 📄 License

This project is open-source and free to use for personal/educational purposes.

---

*Built for desktop aesthetics, open-source spirit, and the sheer joy of hardware hacking.*
