import serial
import psutil
import pyautogui
import time
import asyncio
import pynvml
import wmi
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
            
            song = f"{info.artist} - {info.title}" if info.artist and info.title else (info.title or "Unknown")
            
            pos = timeline.position.total_seconds()
            end = timeline.end_time.total_seconds()
            progress = int((pos / end) * 100) if end > 0 else 0
            
            return (song, progress)
        return ("No music playing", 0)
    except Exception:
        return ("Media error", 0)

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

last_update = 0

print("Windows Dashboard link active. Press Ctrl+C to stop.")
while True:
    handle_commands()
    
    current_time = time.time()
    if current_time - last_update > 1.0: # Send data to LCD every 1 second
        last_update = current_time
        
        sys_line1 = get_cpu_ram()
        sys_line2 = get_temp_gpu()
        song_txt, progress = get_spotify()
        
        # Package the data and send it over COM3
        data_packet = f"<{sys_line1}|{sys_line2}|{song_txt}|{progress}>\n"
        arduino.write(data_packet.encode('utf-8'))
        
    time.sleep(0.05)