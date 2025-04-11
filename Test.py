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

    print("Starting CAN dump (Ctrl+C to stop)...")
    while True:
        try:
            line = ser.readline().decode().strip()

            # Extract ID, Length, and Data using regex
            match = re.match(r"ID:(0x[0-9A-Fa-f]+),L:(\d+),D:(.*)", line)
            if match:
                can_id = match.group(1)  # CAN ID (e.g., 0x540)
                length = int(match.group(2))  # Data Length (e.g., 8)
                data = match.group(3).strip()  # Data (e.g., 0x2 0x0 0x0 0x0 0x11 0x0 0x0 0x77)

                print(f"Extracted - CAN ID: {can_id}, Length: {length}, Data: {data}")
            else:
                print(f"No match: {line}")  # Print the line if it doesn't match the expected format

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
