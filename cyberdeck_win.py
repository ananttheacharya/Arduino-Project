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

current_track_id = ""
lyrics_data = [] # List of tuples (time_in_seconds, text)
current_lyric_text = ""

def fetch_lyrics_bg(artist, title):
    global lyrics_data
    try:
        url = "https://lrclib.net/api/search?q=" + urllib.parse.quote(f"{title} {artist}")
        req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            for d in data:
                if d.get('syncedLyrics'):
                    parse_lrc(d.get('syncedLyrics'))
                    return
        lyrics_data = []
    except:
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

async def get_spotify_async():
    try:
        sessions = await MediaManager.request_async()
        current_session = sessions.get_current_session()
        if current_session:
            info = await current_session.try_get_media_properties_async()
            timeline = current_session.get_timeline_properties()
            pb = current_session.get_playback_info()
            
            song_id = f"{info.artist} - {info.title}" if info.artist and info.title else (info.title or "")
            artist = info.artist
            title = info.title
            
            pos = timeline.position.total_seconds()
            end = timeline.end_time.total_seconds()
            progress = int((pos / end) * 100) if end > 0 else 0
            
            return (song_id, artist, title, progress, pos)
        return ("No music playing", "", "", 0, 0.0)
    except Exception:
        return ("Media error", "", "", 0, 0.0)

def get_spotify():
    # Helper to run the async Windows API call in our synchronous loop
    return asyncio.run(get_spotify_async())


def handle_commands():
    if arduino.in_waiting > 0:
        cmd = arduino.readline().decode('utf-8').strip()
        
        # Uses pyautogui to simulate hitting media keys on a Windows keyboard
        if cmd == "PLAY_PAUSE":
            pyautogui.press('playpause')
        elif cmd == "NEXT":
            pyautogui.press('nexttrack')
        elif cmd == "PREV":
            pyautogui.press('prevtrack')

last_sys_update = 0
last_spotify_update = 0
sys_line1 = get_cpu_ram()
sys_line2 = get_temp_gpu()
song_txt = "Waiting..."
progress = 0

print("Windows Dashboard link active. Press Ctrl+C to stop.")
while True:
    handle_commands()
    
    current_time = time.time()
    
    # 1. Fast loop for Spotify and Lyrics (10Hz)
    if current_time - last_spotify_update > 0.1:
        last_spotify_update = current_time
        
        song_id, artist, title, progress, pos = get_spotify()
        
        if song_id != current_track_id:
            current_track_id = song_id
            lyrics_data = []
            current_lyric_text = ""
            if artist and title:
                threading.Thread(target=fetch_lyrics_bg, args=(artist, title)).start()

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
            arduino.write(f"<L|{current_lyric_text}>\n".encode('utf-8'))
            
        song_txt = song_id if song_id else "No music playing"

    # 2. Slow loop for System Stats and main packet (1Hz)
    if current_time - last_sys_update > 1.0: 
        last_sys_update = current_time
        
        sys_line1 = get_cpu_ram()
        sys_line2 = get_temp_gpu()
        
        data_packet = f"<{sys_line1}|{sys_line2}|{song_txt}|{progress}>\n"
        arduino.write(data_packet.encode('utf-8'))
        
    time.sleep(0.05)