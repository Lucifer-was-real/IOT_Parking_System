import firebase_admin
from firebase_admin import credentials, db
from flask import Flask, request
import easyocr
import cv2
import numpy as np
import time
import socket  # Needed for wireless communication with ESP8266

# --- 1. CONFIGURATION ---
# IMPORTANT: Find the IP of your ESP8266 from its Serial Monitor and put it here
ESP8266_IP = "192.168.0.114" 
UDP_PORT = 4210

# --- 2. FIREBASE SETUP ---
try:
    cred = credentials.Certificate("serviceAccountKey.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://esp32-cam-ocr-dcdd1-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })
    ref = db.reference('/vehicle_logs')
    print("--- Firebase Connected ---")
except Exception as e:
    print(f"Firebase Error: {e}")

# --- 3. OCR SETUP ---
print("Loading OCR Engine (EasyOCR)...")
# verbose=False prevents the 'charmap' progress bar crash on Windows
reader = easyocr.Reader(['en'], verbose=False) 
print("--- OCR Engine Ready ---")

# --- 4. UDP SENDER FUNCTION ---
def send_to_esp8266_display(text):
    """Sends the detected plate number wirelessly to the ESP8266 LCD"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(text.encode(), (ESP8266_IP, UDP_PORT))
        print(f"Wireless Send to ESP8266: {text}")
    except Exception as e:
        print(f"UDP Broadcast Error: {e}")

# --- 5. FLASK SERVER ---
app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    try:
        # Receive image from ESP32-CAM
        file = request.files['image']
        img_bytes = np.frombuffer(file.read(), np.uint8)
        img = cv2.imdecode(img_bytes, cv2.IMREAD_COLOR)

        if img is None:
            return "Decode Failed", 400

        # --- IMAGE PRE-PROCESSING ---
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        
        # Adaptive Thresholding creates high-contrast B&W for the AI
        processed_img = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
                                              cv2.THRESH_BINARY, 11, 2)

        # --- PERFORM OCR ---
        results = reader.readtext(processed_img, detail=0)
        
        # Clean the text: remove spaces and make uppercase
        plate_text = "".join(results).replace(" ", "").strip().upper()

        # If we found a string long enough to be a plate (usually > 4 chars)
        if len(plate_text) > 4:
            print(f"\n[{time.strftime('%H:%M:%S')}] FINAL PLATE: {plate_text}")
            
            # STEP A: Push to Firebase Database
            try:
                ref.push({
                    'plate_number': plate_text,
                    'timestamp': {".sv": "timestamp"}
                })
            except:
                print("Firebase Push Failed (check internet)")

            # STEP B: Send wirelessly to ESP8266 Display
            send_to_esp8266_display(plate_text)
            
            return plate_text, 200
        
        return "No clear text found", 200

    except Exception as e:
        print(f"Server Error: {e}")
        return str(e), 500

if __name__ == '__main__':
    print("\n--- SERVER RUNNING ---")
    print("Ensure ESP32-CAM points to this PC's IP address.")
    app.run(host='0.0.0.0', port=5000)