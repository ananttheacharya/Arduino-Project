# 🖥️ Cyberdeck V3 — The "Why did my Arduino deserve this??" Edition

Welcome to the **Cyberdeck**, an over-engineered, unnecessarily complex **always-on desktop companion** and **offline arcade machine** powered by an **Arduino Uno** and a humble **16×2 I²C LCD**. It runs off 2KB of RAM. 

This little Frankenstein's monster displays live PC stats, syncs real-time Spotify LRC lyrics with a typewriter animation, detects actual literal house fires, and—when unplugged—transforms into an offline battery-powered gaming console that plays Chrome Dino, Simon Says, and a fully functional "Who Wants To Be A Millionaire" trivia game synced straight to the Arduino's non-volatile EEPROM.

Because loving yourself is cringe.

---

## 📸 Feature Overview

| Mode | State | What you see |
|---|---|---|
| **PC Stats** | PC `🔌` | CPU %, RAM %, CPU temp, GPU temp. For when you need to know Chrome is eating 12GB of RAM. |
| **Spotify** | PC `🔌` | Scrolling song/artist name + hyper-smooth 80-segment sub-pixel progress bar. |
| **Lyrics** | PC `🔌` | Typewriter-animated, time-synced LRC lyrics. Karaoke for ants. |
| **Wall-E Eyes** | PC `🔌` / Batt `🔋` | Ambient blinking robot eyes that judge you and periodically tell you to "Drink Water" or "Fix Posture". |
| **Cyber Runner** | Batt `🔋` | The Chrome Dino game, but cooler. Sub-grid pixel rendering completely eliminates LCD ghosting. |
| **Simon Says** | Batt `🔋` | Memorize the sequence. Mess up and the Arduino laughs at your score. |
| **Trivia Master** | Batt `🔋` | 10 multiple-choice trivia questions synced from the web to the Arduino's EEPROM. Includes Lifelines! |
| **🔥 Flame Alert** | ANY | A literal flame sensor that triggers a Windows PowerShell script to blast an MP3 alarm if your desk catches fire. |

---

## 🎮 The "Unplugged" Battery Gaming Console

When you rip the USB cord out of your PC, the Arduino realizes it has been abandoned. it enters **Battery Mode** and boots up the arcade. 

### 🏃‍♂️ Cyber Runner
A fully-fledged Chrome Dino clone. Uses aggressive memory optimization and `PROGMEM` custom sprites. I had to invent sub-grid targeting to overwrite specific characters to stop the LCD liquid crystals from melting your retinas with ghosting. 
- **D2**: Jump. 

### 🧠 Simon Says
A classic memory game. Watch the screen flash the buttons (`> D2 <`, `> D5 <`, etc.) and repeat them. boring

### ❓ Trivia Game (With EEPROM Syncing), my personal fav
Every time you connect the Arduino to your PC, a background Python thread silently pillages the Open Trivia Database API for 10 fresh, medium-difficulty questions and burns them directly into the Arduino's flash memory. When you unplug the deck and take it on a road trip, those questions go with you.
- **D5 / D8**: Cursor-scroll left and right through the options. 
- **D2**: Lock in your final answer.
- **D6**: Open the **Lifelines** Menu!
  - 📞 **Phone a Friend**: Get incredibly unhelpful, quirky messages from your friends. im sorry guys
  - 👥 **Audience Poll**: Analyzes the crowd. 99% accurate. 1% chance it completely lies to you and ruins your run.
  - ✂️ **50/50**:  deletes two incorrect options from the UI. The scrolling cursor is smart enough to skip the dead options.

---

## 🗂️ Project Structure

```text
Arduino Project/
├── sketch_jun3a/
│   └── sketch_jun3a.ino      # 2KB RAM? Challenge Accepted. (C++)
├── cyberdeck_win.py           # Windows host companion script (Python)
├── MKBAAG.mp3                 # Desk on fire? This plays. MKB AAG
└── ...                        # Bunch of utility test scripts
```

---

## 🔌 Hardware & Wiring 

### 16×2 I²C LCD (PCF8574 Backpack)
Address is `0x27`. Four wires (VCC, GND, SDA on A4, SCL on A5). That's it.

### Buttons (Active LOW, Internal Pullups)
Wire them straight from the digital pin to GND. 

| Button | Arduino Pin | PC Mode Action | Battery Mode Action |
|---|---|---|---|
| **BTN_A** | `D2` | Next Screen (Short) / Play/Pause (Long) | Cyber Runner Jump / Confirm Trivia |
| **BTN_B** | `D8` | Next Track | Right Cursor / Simon Input |
| **BTN_C** | `D6` | Custom Extensible Action | Open Trivia Lifelines Menu |
| **BTN_D** | `D5` | Custom Extensible Action | Left Cursor / Simon Input |

### Flame / IR Sensor Module
Digital Out to `D7`. Senses fire. Sends `FLAME_DETECTED` to PC. PC launches PowerShell and screams at you. Modern engineering.

---

## 💻 How the System Actually Works

The architecture is a brilliant mess of asynchronous Python and real-time C++ looping.

1. **Python Host (`cyberdeck_win.py`)**: Uses a multithreaded architecture wrapped in an `asyncio` loop. It queries Windows WinRT APIs for media states, `psutil` for CPU/RAM, and WMI for thermals. 
2. **Thread Locking**: The script uses a strict `threading.Lock()` mutex because PySerial on Windows absolutely implodes if you try to send a Trivia packet and a Spotify packet at the exact same millisecond.
3. **Arduino (`sketch_jun3a.ino`)**: Runs a highly optimized 10ms loop. We had to offload all the custom pixel-art sprites (Dino, Cactus, Phone logo, Audience logo, 50/50 logo, blinking eyes) into `PROGMEM` flash storage because the ATmega328P's RAM fills up if you look at it wrong. The screen only draws what strictly needs to be drawn to prevent the LCD driver from thrashing.

---

## 🚀 Setup & Installation

### 1. Flash the Arduino
Install the **LiquidCrystal_I2C** library. Upload `sketch_jun3a.ino` to your Uno/Nano. Close the Serial Monitor so Python can hijack the COM port.

### 2. Python Dependencies
You need basically every Windows systems library in existence:
```bash
pip install pyserial psutil pynvml wmi pyautogui winrt-runtime unidecode
```

### 3. Run It
Edit `SERIAL_PORT = 'COM3'` in the python script to match your setup, and run it:
```bash
python cyberdeck_win.py
```
> **Pro Tip:** Run as Administrator or the WMI thermal sensors will just output `--` because Windows doesn't trust you with ACPI temperatures.

---

## ⚠️ Troubleshooting

- **`Write timeout` Error**: Your PC is too fast and the Arduino is too slow. The Arduino takes 3 full seconds to boot and 300ms to flash EEPROM. The Python script is now hardcoded to wait for it.
- **LCD Flashing / Ghosting**: Fixed in V3! We stopped thrashing the LCD bus with unnecessary redraws.
- **No Trivia Data**: Your PC failed to hit the `opentdb.com` API. Don't worry, the Python script will automatically inject a hardcoded offline fallback list of trivia into the EEPROM so you're never bored.

---

*Built because doing things the easy way is boring. Open-source.*
