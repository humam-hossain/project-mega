import serial
import time

# Replace 'COMx' (Windows) or '/dev/ttyUSB0' (Linux/Mac) with your Arduino's serial port
arduino_port = '/dev/ttyUSB0'  # Example for Linux
baud_rate = 115200  # Match the Arduino's baud rate

try:
    # Open serial connection
    ser = serial.Serial(arduino_port, baud_rate, timeout=1)
    print(f"Connected to {arduino_port}")
    
    time.sleep(2)  # Allow time for the connection to establish
    
    while True:
        # Read data from the Arduino
        if ser.in_waiting > 0:
            # Decode data, replacing invalid characters
            data = ser.readline().decode('utf-8', errors='replace').strip()
            print(f"{data}")
        
except serial.SerialException as e:
    print(f"Error: {e}")
except KeyboardInterrupt:
    print("\nExiting...")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")
