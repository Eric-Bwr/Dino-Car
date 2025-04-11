import serial.tools.list_ports
import serial
import re
from datetime import datetime

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

    with open("canlog.txt", "a") as log_file:
        print("Starting CAN dump (Ctrl+C to stop)...")
        log_file.write("Timestamp,CAN ID,Length,Data\n")  # CSV header

        while True:
            try:
                line = ser.readline().decode().strip()
                match = re.match(r"ID:(0x[0-9A-Fa-f]+),L:(\d+),D:(.*)", line)

                if match:
                    can_id = match.group(1)
                    length = int(match.group(2))
                    data = match.group(3).strip()

                    # Add microseconds to timestamp
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")

                    # Console output
                    print(f"{timestamp} - CAN ID: {can_id}, Length: {length}, Data: {data}")

                    # File logging
                    log_file.write(f"{timestamp},{can_id},{length},{data}\n")
                else:
                    print(f"No match: {line}")

            except KeyboardInterrupt:
                print("\nStopping CAN dump...")
                break
            except Exception as e:
                print(f"Error: {str(e)}")

except Exception as e:
    print(f"Initialization error: {str(e)}")
finally:
    if 'ser' in locals():
        ser.close()
        print("Serial connection closed")
