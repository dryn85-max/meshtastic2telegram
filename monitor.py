#!/usr/bin/env python3
import serial
import time

port = "/dev/tty.usbserial-0001"
baud = 115200

print(f"Opening {port} at {baud} baud...")
print("Press Ctrl+C to exit\n")

try:
    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(0.5)  # Wait for connection
    
    print("=" * 60)
    print("Serial Monitor Started - Waiting for output...")
    print("=" * 60)
    print()
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').rstrip()
            if line:
                print(line)
                
except KeyboardInterrupt:
    print("\n\nMonitoring stopped by user")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals():
        ser.close()
    print("Serial port closed")

