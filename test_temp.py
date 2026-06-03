import psutil
try:
    print("psutil sensors:", psutil.sensors_temperatures())
except Exception as e:
    print("psutil failed:", e)

try:
    import wmi
    w = wmi.WMI(namespace="root\\wmi")
    temperature_info = w.MSAcpi_ThermalZoneTemperature()
    for temp in temperature_info:
        print("WMI ThermalZone:", temp.InstanceName, (temp.CurrentTemperature / 10.0) - 273.15)
except Exception as e:
    print("WMI failed:", e)

try:
    import subprocess
    output = subprocess.check_output(['powershell', '-Command', 'Get-CimInstance -Namespace root/wmi -ClassName MSAcpi_ThermalZoneTemperature | Select-Object InstanceName, CurrentTemperature'])
    print("CIM Output:")
    print(output.decode('utf-8'))
except Exception as e:
    print("CIM failed:", e)
