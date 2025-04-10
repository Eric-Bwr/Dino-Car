import serial.tools.list_ports
import serial
from datetime import datetime

def find_arduino_port():
    for port in serial.tools.list_ports.comports():
        if "Arduino" in port.manufacturer or "Arduino" in port.description:
            return port.device
    raise Exception("Kein Arduino gefunden!")

try:
    arduino_port = find_arduino_port()
    ser = serial.Serial(arduino_port, 115200, timeout=1)
    print(f"Arduino gefunden an {arduino_port}")

    while True:
        try:
            line = ser.readline().decode().strip()
            if line:
                parts = line.split(',')
                can_id = parts[0].upper()
                length = int(parts[1])
                data = parts[2:2+length]

                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                data_str = ' '.join([f"{byte.zfill(2)}" for byte in data])

                print(f"{timestamp}  {can_id}   [{length}]  {data_str}")

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Fehler: {e}")

except Exception as e:
    print(str(e))
finally:
    if 'ser' in locals():
        ser.close()
