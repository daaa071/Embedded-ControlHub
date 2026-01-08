"""
serial_console.py

Simple Linux serial console for communicating with STM32.
Supports sending text commands and logging sensor data to a file.
"""

import serial
from datetime import datetime

# -------------------------------------------------------------------------
# User Configuration
# -------------------------------------------------------------------------

# Serial port to which the STM32 is connected.
# On Linux, this is usually /dev/ttyACM0 or /dev/ttyUSB0
# On Windows, it might be COM5, COM6, etc.
SERIAL_PORT = "/dev/ttyACM0"

# File where sensor data will be logged
LOG_FILE = "sensors.txt"

# -----------------------------------------------------------------------------
# Serial Configuration
# -----------------------------------------------------------------------------

PORT = SERIAL_PORT        # Linux example
BAUD = 115200
TIMEOUT = 1          # Serial read timeout in seconds

# -----------------------------------------------------------------------------
# Files
# -----------------------------------------------------------------------------

SENSORS_FILE = LOG_FILE

# -----------------------------------------------------------------------------
# Main Application
# -----------------------------------------------------------------------------

def main():
    """
    Opens the serial port, sends user commands to STM32,
    prints responses, and logs sensor data to a file.
    """

    # Try to open the serial port
    try:
        ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    except serial.SerialException as e:
        print("❌ Failed to open COM port:", e)
        return

    print("✅ STM32 connected")
    print("👉 Enter command (SENSORS / STOP / RELAY ON / RELAY OFF / STEPPER MOVE ...)")
    print("👉 Ctrl+C — exit\n")

    try:
        while True:
            # Read command from user
            cmd = input(">>> ").strip()
            if not cmd:
                continue

            # Send command to STM32
            ser.write((cmd + "\n").encode())

            # Read responses from STM32
            while True:
                line = ser.readline().decode(errors="ignore").strip()
                if not line:
                    break

                # If this is sensor data, append it to the log file
                if line.startswith("T=") or line.startswith("H=") or "P=" in line:
                    with open(SENSORS_FILE, "a") as f:
                        f.write(f"{datetime.now()} {line}\n")
                else:
                    # Print all other messages to console
                    print(line)

    except KeyboardInterrupt:
        print("\n👋 Exit")

    # Close serial port on exit
    ser.close()

# -----------------------------------------------------------------------------
# Entry Point
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    main()
