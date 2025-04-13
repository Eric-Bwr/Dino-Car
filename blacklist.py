import serial.tools.list_ports
import serial
import re
import tkinter as tk
from tkinter import ttk
import tkinter.font as tkFont

def find_arduino_port():
    for port in serial.tools.list_ports.comports():
        if (port.manufacturer and "Arduino" in port.manufacturer) or \
                (port.description and "Arduino" in port.description):
            return port.device
    available_ports = ", ".join(p.device for p in serial.tools.list_ports.comports())
    raise Exception(f"No Arduino found! Available ports: {available_ports}")

def parse_can_data(data_string):
    """Parses the data string into a list of decimal values with 3 digits."""
    try:
        hex_values = data_string.split()
        return [f"{int(hex_value, 16):03d}" for hex_value in hex_values]  # Convert to decimal with 3 digits
    except:
        return None

def update_gui():
    try:
        line = ser.readline().decode().strip()
        match = re.match(r"ID:(0x[0-9A-Fa-f]+),L:(\d+),D:(.*)", line)
        if match:
            can_id = match.group(1)
            length = int(match.group(2))
            data_string = match.group(3).strip()
            data = parse_can_data(data_string)

            if data is not None:
                # Create a label if it doesn't exist
                if can_id not in id_labels:
                    label = ttk.Label(root, text=f"{can_id}: Waiting for data...", font=fontStyle)  # Apply font style
                    label.pack(pady=5)
                    id_labels[can_id] = label

                # Update the label in the GUI
                id_labels[can_id].config(text=f"Data: {str(data)}")
    except Exception as e:
        print(f"Error reading or parsing serial data: {e}")

    root.after(100, update_gui)  # Schedule the function to be called every 100 ms

try:
    arduino_port = find_arduino_port()
    ser = serial.Serial(arduino_port, 9600, timeout=1)
    print(f"Connected to Arduino at {arduino_port}")

    # Initialize GUI
    root = tk.Tk()
    root.title("CAN Data Monitor")

    # Font Style Configuration
    fontStyle = tkFont.Font(family="Lucida Grande", size=12)  # You can change family and size

    # Dictionary to store labels for each CAN ID
    id_labels = {}

    # No longer predefine TARGET_IDS

    can_data = {} # Store last can data

    # Start updating the GUI
    root.after(1, update_gui)
    root.mainloop()

except Exception as e:
    print(f"Initialization error: {e}")
finally:
    if 'ser' in locals():
        ser.close()
        print("Serial connection closed")
