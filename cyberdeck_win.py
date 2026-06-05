import serial
import psutil
import pyautogui
import time
import asyncio
import pynvml
import wmi
import urllib.request
import urllib.parse
import json
import threading
import re
import os
import subprocess
import html
import random

serial_lock = threading.Lock()

try:
    from unidecode import unidecode
except ImportError:
    # Fallback if the user hasn't installed unidecode
    def unidecode(text):
        return text.encode('ascii', 'ignore').decode('ascii')

current_track_id = ""
lyrics_data = [] # List of tuples (time_in_seconds, text)
current_lyric_text = ""

def clean_title(title):
    # Just a simple string split to remove " - Remastered" etc.
    return title.split('-')[0].strip()

def fetch_lyrics_bg(artist, title):
    global lyrics_data
    # Use one single query to avoid getting rate-limited or timing out
    q = f"{clean_title(title)} {artist}"
    try:
        url = "https://lrclib.net/api/search?q=" + urllib.parse.quote(q)
        req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            for d in data:
                if d.get('syncedLyrics'):
                    parse_lrc(d.get('syncedLyrics'))
                    return
        lyrics_data = []
    except Exception:
        lyrics_data = []

def parse_lrc(lrc_str):
    global lyrics_data
    parsed = []
    for line in lrc_str.split('\n'):
        line = line.strip()
        if line.startswith('[') and ']' in line:
            time_part = line[1:line.find(']')]
            text_part = line[line.find(']')+1:].strip()
            try:
                mins, secs = time_part.split(':')
                total_sec = int(mins) * 60 + float(secs)
                if text_part:
                    parsed.append((total_sec, text_part))
            except:
                pass
    parsed.sort(key=lambda x: x[0])
    lyrics_data = parsed

# CHANGED: We are using winrt now instead of winsdk to avoid the C++ build errors
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager

# Updated for Windows
SERIAL_PORT = 'COM3' 
BAUD_RATE = 9600

try:
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1, write_timeout=2.0)
    time.sleep(4) # Wait for Arduino to initialize (bootloader + setup delay)
    print(f"Successfully connected to {SERIAL_PORT}")
except Exception as e:
    print(f"Failed to connect to {SERIAL_PORT}: {e}")
    print("Make sure the Arduino IDE Serial Monitor is CLOSED.")
    exit(1)

def fetch_trivia():
    try:
        url = "https://opentdb.com/api.php?amount=10&difficulty=medium&type=multiple"
        req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            results = data.get("results", [])
    except Exception as e:
        print(f"Network error ({e}). Using offline fallback trivia!")
        results = [
            {"question": "What is the capital of France?", "correct_answer": "Paris", "incorrect_answers": ["London", "Berlin", "Rome"]},
            {"question": "Which planet is known as the Red Planet?", "correct_answer": "Mars", "incorrect_answers": ["Venus", "Jupiter", "Saturn"]},
            {"question": "What is the largest mammal?", "correct_answer": "Blue Whale", "incorrect_answers": ["Elephant", "Giraffe", "Orca"]},
            {"question": "Who painted the Mona Lisa?", "correct_answer": "Da Vinci", "incorrect_answers": ["Van Gogh", "Picasso", "Monet"]},
            {"question": "What is the chemical symbol for Gold?", "correct_answer": "Au", "incorrect_answers": ["Ag", "Fe", "Cu"]},
            {"question": "Which language is most spoken worldwide?", "correct_answer": "Mandarin", "incorrect_answers": ["English", "Spanish", "Hindi"]},
            {"question": "How many continents are there?", "correct_answer": "7", "incorrect_answers": ["5", "6", "8"]},
            {"question": "What gas do plants absorb?", "correct_answer": "CO2", "incorrect_answers": ["Oxygen", "Nitrogen", "Helium"]},
            {"question": "Who wrote 'Hamlet'?", "correct_answer": "Shakespeare", "incorrect_answers": ["Dickens", "Homer", "Tolkien"]},
            {"question": "What is the square root of 144?", "correct_answer": "12", "incorrect_answers": ["10", "14", "16"]}
        ]

    try:
        for idx, q in enumerate(results):
            question = html.unescape(q["question"])[:44]
            correct_ans = html.unescape(q["correct_answer"])[:9]
            incorrect = [html.unescape(x)[:9] for x in q["incorrect_answers"]]
            
            options = incorrect + [correct_ans]
            random.shuffle(options)
            correct_idx = options.index(correct_ans)
            
            q_text = unidecode(question)
            opts = [unidecode(opt) for opt in options]
            
            packet = f"<Q|{idx}|{q_text}|{opts[0]}|{opts[1]}|{opts[2]}|{opts[3]}|{correct_idx}>\n"
            with serial_lock:
                arduino.write(packet.encode('utf-8'))
            time.sleep(0.4) # Give Arduino 300ms+ to complete EEPROM write
        print("Trivia synced to EEPROM successfully!")
    except Exception as e:
        print("Failed to sync trivia:", e)

def get_cpu_ram():
    cpu = psutil.cpu_percent(interval=None)
    ram = psutil.virtual_memory().percent
    return f"CPU:{int(cpu)}% RAM:{int(ram)}%".ljust(16)

try:
    pynvml.nvmlInit()
    nvml_inited = True
except:
    nvml_inited = False

wmi_w = None
wmi_access_denied = False
try:
    wmi_w = wmi.WMI(namespace="root\\wmi")
    # Test read to check for admin privileges
    _ = wmi_w.MSAcpi_ThermalZoneTemperature()
except:
    wmi_access_denied = True

def get_temp_gpu():
    cpu_t_str = "--"
    if not wmi_access_denied and wmi_w is not None:
        try:
            temps = wmi_w.MSAcpi_ThermalZoneTemperature()
            if temps:
                cpu_t = int((temps[0].CurrentTemperature / 10.0) - 273.15)
                cpu_t_str = str(cpu_t)
        except:
            pass

    gpu_t_str = "--"
    if nvml_inited:
        try:
            handle = pynvml.nvmlDeviceGetHandleByIndex(0)
            gpu_t = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
            gpu_t_str = str(gpu_t)
        except:
            pass
            
    # Format to fit the 16 character width limit on the LCD
    output = f"C:{cpu_t_str}C G:{gpu_t_str}C"
    return output.ljust(16)

def handle_commands():
    if arduino.in_waiting > 0:
        cmd = arduino.readline().decode('utf-8').strip()
        
        if cmd == "PLAY_PAUSE":
            pyautogui.press('playpause')
        elif cmd == "NEXT":
            pyautogui.press('nexttrack')
        elif cmd == "PREV":
            pyautogui.press('prevtrack')
        elif cmd == "FLAME_DETECTED":
            try:
                script_dir = os.path.dirname(os.path.abspath(__file__))
                mp3_path = os.path.join(script_dir, 'MKBAAG.mp3')
                # Use Windows built-in MediaPlayer via hidden PowerShell (bypasses pygame audio routing issues)
                ps_script = f"Add-Type -AssemblyName presentationCore; $player = New-Object System.Windows.Media.MediaPlayer; $player.Open('{mp3_path}'); $player.Play(); Start-Sleep -Seconds 30"
                subprocess.Popen(["powershell", "-WindowStyle", "Hidden", "-Command", ps_script])
                print(f"🔥 Flame detected! Playing audio via Windows native player...")
            except Exception as e:
                print(f"Error playing sound: {e}")
        elif cmd == "BTN_C_SHORT":
            print("Button C (D6) short press")
        elif cmd == "BTN_C_LONG":
            print("Button C (D6) long press")
        elif cmd == "BTN_D_SHORT":
            print("Button D (D5) short press")
        elif cmd == "BTN_D_LONG":
            print("Button D (D5) long press")

async def main():
    global current_track_id, lyrics_data, current_lyric_text
    
    last_sys_update = 0
    last_spotify_update = 0
    sys_line1 = get_cpu_ram()
    sys_line2 = get_temp_gpu()
    song_txt = "Waiting..."
    safe_song_txt = "Waiting..."
    progress = 0
    pos = 0.0
    
    # Sync trivia questions to Arduino on startup
    threading.Thread(target=fetch_trivia, daemon=True).start()
    
    print("Windows Dashboard link active. Press Ctrl+C to stop.")
    while True:
        handle_commands()
        
        current_time = time.time()
        
        # 1. Fast loop for Timeline and Lyrics Sync (10Hz)
        if current_time - last_spotify_update > 0.1:
            last_spotify_update = current_time
            
            try:
                sessions = await MediaManager.request_async()
                current_session = sessions.get_current_session()
                if current_session:
                    timeline = current_session.get_timeline_properties()
                    pos = timeline.position.total_seconds()
                    end = timeline.end_time.total_seconds()
                    progress = int((pos / end) * 100) if end > 0 else 0
            except Exception:
                pass
                
            new_text = ""
            if lyrics_data:
                for t_sec, text in lyrics_data:
                    # 0.4s lookahead so the typewriter finishes right as the singer speaks
                    if (pos + 0.4) >= t_sec:
                        new_text = text
                    else:
                        break
                        
            if new_text != current_lyric_text:
                current_lyric_text = new_text
                safe_lyric = unidecode(current_lyric_text)
                with serial_lock:
                    arduino.write(f"<L|{safe_lyric}>\n".encode('utf-8'))
                
        # 2. Slow loop for System Stats, Media Metadata, and main packet (1Hz)
        if current_time - last_sys_update > 1.0: 
            last_sys_update = current_time
            
            sys_line1 = get_cpu_ram()
            sys_line2 = get_temp_gpu()
            
            try:
                sessions = await MediaManager.request_async()
                current_session = sessions.get_current_session()
                if current_session:
                    # wait_for prevents YouTube or Chrome from freezing the entire script if they hang
                    info = await asyncio.wait_for(current_session.try_get_media_properties_async(), timeout=0.5)
                    song_id = f"{info.artist} - {info.title}" if info.artist and info.title else (info.title or "")
                    artist = info.artist
                    title = info.title
                else:
                    song_id, artist, title = "No music playing", "", ""
            except Exception:
                song_id, artist, title = "Media error", "", ""
                
            if song_id != current_track_id and song_id != "Media error" and song_id != "No music playing":
                current_track_id = song_id
                lyrics_data = []
                current_lyric_text = ""
                if artist and title:
                    threading.Thread(target=fetch_lyrics_bg, args=(artist, title)).start()
                    
            song_txt = song_id if song_id else "No music playing"
            safe_song_txt = unidecode(song_txt)
            
            data_packet = f"<{sys_line1}|{sys_line2}|{safe_song_txt}|{progress}>\n"
            with serial_lock:
                arduino.write(data_packet.encode('utf-8'))
            
        await asyncio.sleep(0.05)

if __name__ == "__main__":
    asyncio.run(main())