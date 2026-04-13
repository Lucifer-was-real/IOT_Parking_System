#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 1. WiFi Credentials
const char* ssid = "Pranoy";
const char* password = "Pranoy2021$$";

// 2. OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Reset pin not used on most 0.96" OLEDs (-1)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 3. UDP Setup
WiFiUDP Udp;
unsigned int localUdpPort = 4210; 
char incomingPacket[255];

void setup() {
  Serial.begin(115200); // Changed to 115200 for better speed
  
  // Initialize I2C for OLED (Standard pins: D2=SDA, D1=SCL)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nESP8266 Connected!");
  
  // Show IP on OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("CONNECTED!");
  display.println("");
  display.println("IP Address:");
  display.println(WiFi.localIP());
  display.display();

  Udp.begin(localUdpPort);
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    String plate = String(incomingPacket);
    Serial.println("Received Plate: " + plate);

    // Update OLED Display
    display.clearDisplay();
    
    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("VEHICLE DETECTED:");
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE); // Draw a line
    
    // Plate Number (Larger Text)
    display.setTextSize(2); 
    display.setCursor(0, 25);
    display.println(plate);
    
    display.display(); // Push to screen
  }
}