import struct

import serial.tools.list_ports
import serial
import re

def find_arduino_port():
    for port in serial.tools.list_ports.comports():
        if (port.manufacturer and "Arduino" in port.manufacturer) or \
                (port.description and "Arduino" in port.description):
            return port.device
    available_ports = ", ".join(p.device for p in serial.tools.list_ports.comports())
    raise Exception(f"No Arduino found! Available ports: {available_ports}")

try:
    arduino_port = find_arduino_port()
    ser = serial.Serial(arduino_port, 9600, timeout=1)
    print(f"Connected to Arduino at {arduino_port}")

    while True:
        try:
            line = ser.readline().decode().strip()
            match = re.match(r"ID:(0x[0-9A-Fa-f]+),L:(\d+),D:(.*)", line)
            if match:
                can_id = int(match.group(1), 16)
                length = int(match.group(2))
                data = match.group(3).strip()
                data_bytes = [int(x, 16) for x in data.split()]
                if can_id == 0x540:
                    if data_bytes[0] != 0x02:
                        continue
                    gear = str(data_bytes[3] & 0x0F)
                    print(f"Gear: {gear}", flush=True)

                    engine_rpm = (data_bytes[1] << 8) | data_bytes[2]
                    print(f"Engine RPM: {engine_rpm}", flush=True)

                    coolant_temp = struct.unpack(">H", bytes(data_bytes[6:8]))[0] / 10.0
                    print(f"Coolant Temperature: {coolant_temp} °C", flush=True)

        except KeyboardInterrupt:
            print("\nStopping CAN dump...")
            break
        except Exception as e:
            print(f"Error processing message: {str(e)}")

except Exception as e:
    print(f"Initialization error: {str(e)}")
finally:
    if 'ser' in locals():
        ser.close()
        print("Serial connection closed")
