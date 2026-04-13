#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include <WebServer.h>

const char* ssid = "Pranoy";
const char* password = "Pranoy2021$$";
const char* pc_ip = "192.168.0.113"; 
const int pc_port = 5000;

// AI-Thinker Pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

void handleCapture() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // --- UPGRADED RESOLUTION
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;          
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) { Serial.println("Cam Fail"); return; }
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1);  //Brightness
  s->set_contrast(s, 2);    //Contrast

  server.on("/capture", handleCapture);
  server.begin();
  Serial.println("\nSystem Ready. IP: " + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 15000) { 
    lastTime = millis();
    captureAndSend();
  }
}

void captureAndSend() {
  Serial.println("\n[Action] Capturing High-Res Frame...");
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) return;

  WiFiClient client;
  if (!client.connect(pc_ip, pc_port)) {
    Serial.println("[Error] Can't reach PC!");
    esp_camera_fb_return(fb);
    return;
  }

  String boundary = "---------------------------123456789";
  String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";
  uint32_t totalLen = head.length() + fb->len + tail.length();

  client.println("POST /upload HTTP/1.1");
  client.println("Host: " + String(pc_ip));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.print("Content-Length: ");
  client.println(totalLen);
  client.println("Connection: close");
  client.println();

  client.print(head);
  client.write(fb->buf, fb->len);
  client.print(tail);

  //time 10s
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) { client.stop(); esp_camera_fb_return(fb); return; }
  }
  
  while(client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && line.indexOf("HTTP") == -1) {
      Serial.println(">>> PLATE DETECTED: " + line);
    }
  }

  client.stop();
  esp_camera_fb_return(fb);
}