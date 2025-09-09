'''
Comoster Wifi Connection
========================
Description: 
    This program will communicate with the Arduino Giga and a
    server. The RPi, will take the sampled data from the arduino via 
    serial communication through the USB port. It also will have a camera
    to capture images of the composter based on commands from the arduino.
    The camera will capture, save, and send the images with a delay so 
    redundant images are not taken.

Author: Jonathan Osburn
Date: July 8, 2025
Version: 2.0

Hardware:
    - Powered USB Hub
    - Arduino Giga Wifi
    - Logitech C270 HD Webcam w/ USB Extender
'''
# ----------------------------------------------------------------------
# libraries
import serial
import time
import requests
import cv2
import os
from datetime import datetime
import json
import threading

# ----------------------------------------------------------------------
# MACROs
SERIAL_PORT = "/dev/ttyACM0"    # check with ls /dev/tty*
BAUD_RATE = 115200      # 9600
SERVER_URL_READINGS = "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/readings"
SERVER_URL_LOADED =   "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/loaded"
SERVER_URL_UNLOADED = "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/unloaded"
SERVER_URL_PICTURES = "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/pictures"
SERVER_URL_BLOWERS =  "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/blowers"
SERVER_URL_PUMP =     "https://txhj3ekbrg.execute-api.us-east-2.amazonaws.com/prod/pump"

DEVICE_ID = "pi_zero_999"

DATA_DIR =      "/home/pi/data/"
READING_DIR =   "/home/pi/data/readings"
LOADED_DIR =    "/home/pi/data/loaded"
UNLOADED_DIR =  "/home/pi/data/unloaded"
PIC_DIR =       "/home/pi/data/pictures"
BLOWERS_DIR =   "/home/pi/data/blowers"
PUMP_DIR =      "/home/pi/data/pump"

DELAY_SECONDS = 20.00
timer = None

# ----------------------------------------------------------------------
# ensure save directory exists
if not os.path.exists(DATA_DIR):
    os.makedirs(DATA_DIR)
    
if not os.path.exists(READING_DIR):
    os.makedirs(READING_DIR)
    
if not os.path.exists(LOADED_DIR):
    os.makedirs(LOADED_DIR)
    
if not os.path.exists(UNLOADED_DIR):
    os.makedirs(UNLOADED_DIR)
    
if not os.path.exists(PIC_DIR):
    os.makedirs(PIC_DIR)

if not os.path.exists(BLOWERS_DIR):
    os.makedirs(BLOWERS_DIR)

if not os.path.exists(PUMP_DIR):
    os.makedirs(PUMP_DIR)


# ----------------------------------------------------------------------
# Function: Initialize serial port for interfacing with Arduino Giga Wifi
# ----------------------------------------------------------------------
def init_serial():
    try:
        # initialize serial ports
        ser = serial.Serial(
            port=SERIAL_PORT,    # check with ls /dev/tty*
            baudrate=BAUD_RATE,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=1
        )
        SERIAL = True
        
        time.sleep(2)   # wait for connection
    except serial.SerialException as e:
        print(f"Serial port error: {e}")
        return
    return False


# ----------------------------------------------------------------------
# Function: save JSON payload to SD card; can be one of 5 types or else 
#   will not be sent.
# ----------------------------------------------------------------------
def store_data(payload,data_type):
    if data_type == "Loaded":
        save_dir = LOADED_DIR
    elif data_type == "Unloaded":
        save_dir = UNLOADED_DIR
    elif data_type == "Readings":
        save_dir = READING_DIR
    elif data_type == "Blowers":
        save_dir = BLOWERS_DIR
    elif data_type == "Pump":
        save_dir = PUMP_DIR
    else:
        save_dir = "Nothing"
        
    print(f"Directory type: {save_dir}.")
        
    if save_dir != "Nothing":
        try:
            # create file name/dir
            timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
            file_path = os.path.join(save_dir, f"{timestamp}_{data_type}_data_.json")
            
            
            # save to folder
            with open(file_path, 'w') as f:
                json.dump(payload, f, indent=2)
            print(f"Saved data to: {file_path}")
            return True
        except IOError as e:
            print(f"Error saving to file: {e}")    
            return False
        
    else:
        print("No valid server directory specified.")



# ----------------------------------------------------------------------
# Function: Capture image of composter, save it, and send it to server
# ----------------------------------------------------------------------
def capture_and_save_and_send_image():
    # initialize webcam
    cap = cv2.VideoCapture(0)   # /dev/video0
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)  # low resolution
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_BRIGHTNESS, 50)    # adjust for low-light
    cap.set(cv2.CAP_PROP_CONTRAST, 50)
        
    try:
        # capture image
        ret,frame = cap.read()
        
        if not ret: 
            print("Error: Failed to capture image")
            return False
                    
        # enhance for low-light
        frame = cv2.convertScaleAbs(frame,alpha=1.2, beta=20)
        
        # generate timestamped filename in the specified directory
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        image_path = os.path.join(PIC_DIR, f"{timestamp}_picture.jpg")
        
        # save image to specified directory
        cv2.imwrite(image_path,frame)
        print(f"Captured image: {image_path}")

        try:
            files = {
                'image': (f"{DEVICE_ID}_{timestamp}.jpg", open(image_path, 'rb'), 'image/jpeg'),
                'deviceId': (None, DEVICE_ID),
                'timestamp': (None, datetime.utcnow().isoformat() + "Z")
            }
            
            response = requests.post(SERVER_URL_PICTURES, files=files)
            response.raise_for_status()
            print(f"Server response for PICTURE: {response.status_code}, {response.text}")
            
        except requests.exceptions.RequestException as e:
            print(f"Image upload failed: {e}")
            
            
    except KeyboardInterrupt:
        print("Program stopped")

    finally:
        cap.release()


# ----------------------------------------------------------------------
# Function: send JSON payload to server; can be one of 3 types or else 
#   will not be sent.
# ----------------------------------------------------------------------
def send_to_server(payload,data_type):
    if data_type == "Loaded":
        server_url = SERVER_URL_LOADED
    elif data_type == "Unloaded":
        server_url = SERVER_URL_UNLOADED
    elif data_type == "Readings":
        server_url = SERVER_URL_READINGS
    elif data_type == "Blowers":
        server_url = SERVER_URL_BLOWERS
    elif data_type == "Pump":
        server_url = SERVER_URL_PUMP
    else:
        server_url = "Nothing"
        
    print(f"Payload type: {server_url}.")
        
    if server_url != "Nothing":
        headers = {'Content-Type': 'application/json'}
        print(f"Sending payload: {payload}")
        try:
            response = requests.post(server_url, json=payload, headers=headers)
            print(f"Server response: {response.status_code}, {response.text}")
            return True
        except requests.exceptions.RequestException as e:
            print(f"Request failed: {e}")
            return True
            
    else:
        print("No valid server directory specified.")
   
# ----------------------------------------------------------------------
# Function: start a timer to set off image saving and sending. Will be 
#   triggered every time a "Loaded" or "Unloaded" signal is given. If 
#   set off again during timer duration, it will reset as to not spam
#   the server. 
# ----------------------------------------------------------------------
def handle_door_open():
    global timer
    
    # cancel any existing timer
    if timer is not None:
        timer.cancel()
        print("Timer reset due to new door open event.")
        
    # start a new timer
    timer = threading.Timer(delay, capture_and_save_and_send_image)
    timer.start()
    print(f"Timer started for {delay} seconds.")


# ----------------------------------------------------------------------
# Function: Main kinda stuff here
# ----------------------------------------------------------------------
def main():
    print("in main")
    # init_serial()
    try:
        # initialize serial ports
        ser = serial.Serial(
            port=SERIAL_PORT,    # check with ls /dev/tty*
            baudrate=BAUD_RATE,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=1
        )
        SERIAL = True
        
        time.sleep(2)   # wait for connection
    except serial.SerialException as e:
        print(f"Serial port error: {e}")
        return
        
    # clear serial buffer
    ser.reset_input_buffer()
    global delay
    delay = DELAY_SECONDS
    print("Init: time set to 5 seconds")
    # clear serial buffer
    ser.reset_input_buffer()
    try:
        while True:
            # line = input("Enter What you would like to do, Ctrl+C to quit: ")
            
            if ser.in_waiting > 0: # wait for serial buffer to be filled
                line = ser.readline().decode('utf-8').strip()
                print(f"\n\nRecieved: {line}\n")

                if line.startswith("Data:"):
                    data_str = line[5:]
                    print(f"--Data--\nRemaining String: {data_str}")

                    try:
                        data = [float(x) for x in data_str.split(',')]
                        if len(data) == 8:
                            # create JSON payload for Lambda
                            payload = {
                                "deviceId": DEVICE_ID,
                                "timestamp": datetime.utcnow().isoformat() + "Z",
                                "sensor": [
                                    {"temp": data[0], "hum": data[1]},
                                    {"temp": data[2], "hum": data[3]},
                                    {"temp": data[4], "hum": data[5]}
                                ],
                                "o2": data[6],
                                "fill": data[7]
                            }
                            print(f"Data: {data[0]}, {data[1]}, {data[2]}, {data[3]}, {data[4]}, {data[5]}, {data[6]}, {data[7]}")
                            
                            # sensor[0] IS ON TOP
                            # sensor[0]temp, sensor[0]hum, sensor[1]temp, sensor[1]hum, sensor[2]temp, sensor[2]hum, o2, fill
                            # Data:1,2,3,4,5,6,7,8
                            
                            # store payload
                            store_data(payload,"Readings")
                            
                            # send to server
                            send_to_server(payload,"Readings")
                            
                        else:
                            print("Error: Expected 8 floats")

                    except ValueError:
                        print("Error: Invalid float format")
                    
                elif line.startswith("Loaded"):
                    payload = {
                                "deviceId": DEVICE_ID,
                                "timestamp": datetime.utcnow().isoformat() + "Z",
                                "loaded": [
                                    {"Status": True}
                                ]
                            }
                            
                    # store payload
                    store_data(payload,"Loaded")
                    
                    # send to server
                    send_to_server(payload,"Loaded")
                    
                    # start 3 min timer for picture
                    handle_door_open()
                        
                elif line.startswith("Unloaded"):
                    payload = {
                                "deviceId": DEVICE_ID,
                                "timestamp": datetime.utcnow().isoformat() + "Z",
                                "unloaded": [
                                    {"Status": True}
                                ]
                            }
                            
                    # store payload
                    store_data(payload,"Unloaded")
                    
                    # send to server
                    send_to_server(payload,"Unloaded")
                    
                    # start 3 min timer for picture
                    handle_door_open()
                    
                elif line.startswith("Delay:"):
                    delay = float(line[6:])
                    print(f"Camera delay changed to {delay} seconds")
                    
                elif line.startswith("Blower:"):
                    print("Blowers Activated!")
                    payload = {
                                "deviceId": DEVICE_ID,
                                "timestamp": datetime.utcnow().isoformat() + "Z",
                                "blower": [
                                    {"Status": True}
                                ]
                            }
                    # store payload
                    store_data(payload,"Blowers")        
                    
                    # send payload
                    send_to_server(payload,"Blowers")
                    
                elif line.startswith("Pump:"):
                    print("Pump Activated!")
                    payload = {
                                "deviceId": DEVICE_ID,
                                "timestamp": datetime.utcnow().isoformat() + "Z",
                                "pump": [
                                    {"Status": True}
                                ]
                            }
                    
                    # store payload
                    store_data(payload,"Pump")        
                    
                    # send payload
                    send_to_server(payload,"Pump")
                    
            time.sleep(0.005)     # avoid overloading

    except KeyboardInterrupt:
        print("Program stopped")

    finally:
        ser.close()
        print("Done :))")
        
# entry point of program
if __name__ == "__main__":
    main()
