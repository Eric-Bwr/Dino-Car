#!/usr/bin/env python3
"""
CAN-IDs:
  0x540: RPM, Gear, Coolant Temp
  0x541: Ambient Temp, Keepalive
  0x12B: Wheel Speed (ABS)

Usage:
  sudo python3 can_publish.py [rpm] [coolant] [ambient] [gear] [speed]
  sudo python3 can_publish.py 5000 85.0 22.5 3 60
  sudo python3 can_publish.py   (Demo-Modus)
"""
import can
import time
import sys

SPEED_FACTOR = 16  # 0.0625 km/h per LSB (Bosch ABS encoding)

def build_0x540(rpm, gear, coolant_temp):
    """Engine frame: [0x02, RPM_H, RPM_L, GEAR, 0, 0, TEMP_H, TEMP_L]"""
    temp_val = int(coolant_temp * 10)
    return [
        0x02,
        (rpm >> 8) & 0xFF,
        rpm & 0xFF,
        gear & 0x0F,
        0x00,
        0x00,
        (temp_val >> 8) & 0xFF,
        temp_val & 0xFF,
    ]

def build_0x541_ambient(ambient_temp):
    """Ambient temp frame: [0x02, 0, 0, 0, 0, 0, TEMP_H, TEMP_L]"""
    temp_val = int(ambient_temp * 10)
    return [
        0x02,
        0x00, 0x00, 0x00, 0x00, 0x00,
        (temp_val >> 8) & 0xFF,
        temp_val & 0xFF,
    ]

def build_0x541_keepalive():
    """Keepalive: [0x06, 0, 0, 0, 0, 0, 0, 0]"""
    return [0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

def build_0x12B(speed_kmh):
    """ABS wheel speed: [SPEED_H, SPEED_L, 0, 0, 0, 0, 0, 0]
       speed_kmh * 16 = CAN value"""
    speed_val = int(speed_kmh * SPEED_FACTOR)
    return [
        (speed_val >> 8) & 0xFF,
        speed_val & 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ]

def send(bus, can_id, data):
    msg = can.Message(arbitration_id=can_id, data=data, is_extended_id=False)
    try:
        bus.send(msg)
    except can.CanOperationError:
        time.sleep(0.01)
        bus.send(msg)

def main():
    bus = can.interface.Bus(channel="can0", interface="socketcan")

    if len(sys.argv) >= 2:
        rpm = int(sys.argv[1])
        coolant = float(sys.argv[2]) if len(sys.argv) >= 3 else 20.0
        ambient = float(sys.argv[3]) if len(sys.argv) >= 4 else 20.0
        gear = int(sys.argv[4]) if len(sys.argv) >= 5 else 0
        speed = float(sys.argv[5]) if len(sys.argv) >= 6 else 0.0
    else:
        rpm, coolant, ambient, gear, speed = 0, 85.0, 22.5, 1, 0.0

    demo = len(sys.argv) < 2
    print(f"{'Demo-Modus' if demo else 'Fixe Werte'}: RPM={rpm} Coolant={coolant}°C Ambient={ambient}°C Gear={gear} Speed={speed}km/h")
    print("Ctrl+C zum Beenden\n")

    try:
        while True:
            # Engine: RPM + Gear + Coolant
            send(bus, 0x540, build_0x540(rpm, gear, coolant))
            send(bus, 0x541, build_0x541_keepalive())
            send(bus, 0x541, build_0x541_ambient(ambient))
            send(bus, 0x541, build_0x541_keepalive())
            # Wheel Speed (ABS) - all ABS IDs to clear ABS error
            send(bus, 0x12A, build_0x12B(speed))
            send(bus, 0x12B, build_0x12B(speed))
            send(bus, 0x12C, build_0x12B(speed))
            send(bus, 0x12D, build_0x12B(speed))
            time.sleep(0.05)

            if demo:
                rpm += 50
                speed = rpm / 100.0
                if rpm > 12000:
                    rpm = 0
                    gear = (gear % 6) + 1

            print(f"RPM: {rpm:5d} | Coolant: {coolant:.1f}°C | Ambient: {ambient:.1f}°C | Gear: {gear} | Speed: {speed:.1f} km/h", end="\r")

    except KeyboardInterrupt:
        print("\nBeendet.")
    finally:
        bus.shutdown()

if __name__ == "__main__":
    main()
