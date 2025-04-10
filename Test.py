import serial.tools.list_ports
import serial
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
    ser = serial.Serial(arduino_port, 115200, timeout=1)
    print(f"Connected to Arduino at {arduino_port}")

    print("Starting CAN dump (Ctrl+C to stop)...")
    while True:
        try:
            line = ser.readline().decode().strip()
            if line:
                parts = line.split(',')
                if len(parts) >= 3:  # Ensure valid message
                    can_id = parts[0].upper().zfill(3)  # 3-digit HEX
                    length = int(parts[1])
                    data = parts[2:2+length]

                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    data_str = ' '.join(f"{byte.zfill(2)}" for byte in data)

                    print(f"{timestamp}  {can_id}   [{length}]  {data_str}")

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
