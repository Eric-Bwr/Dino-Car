import serial.tools.list_ports
import serial

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
            if line.startswith("G:") and ",R:" in line and ",T:" in line:
                parts = line.split(',')
                gear = int(parts[0].split(':')[1])
                rpm = int(parts[1].split(':')[1])
                temp = float(parts[2].split(':')[1])

                print(f"Gear: {gear} | RPM: {rpm} | Temp: {temp}°C", flush=True)

        except KeyboardInterrupt:
            print("\nStopping...")
            break
        except Exception as e:
            print(f"Error: {str(e)}")

except Exception as e:
    print(f"Initialization error: {str(e)}")
finally:
    if 'ser' in locals():
        ser.close()
        print("Serial connection closed")
