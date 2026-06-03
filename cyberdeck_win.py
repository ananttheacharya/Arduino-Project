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
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2) # Wait for Arduino to initialize
    print(f"Successfully connected to {SERIAL_PORT}")
except Exception as e:
    print(f"Failed to connect to {SERIAL_PORT}: {e}")
    print("Make sure the Arduino IDE Serial Monitor is CLOSED.")
    exit(1)

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
            arduino.write(data_packet.encode('utf-8'))
            
        await asyncio.sleep(0.05)

if __name__ == "__main__":
    asyncio.run(main())