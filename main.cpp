// ESP32-CAM main firmware file
// Streams video, receives alphabet from laptop, and displays it

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char* ssid = "LAPTOP";
const char* password = "123456789";

// Flask server details (NO SPACE after http://)
const char* serverName = "http://100.100.1.00:xxxx/detect";

// LCD setup (I2C address 0x27, 16 chars, 2 lines)
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);

  // LCD I2C init on GPIO12 (SDA), GPIO13 (SCL)
  Wire.begin(12, 13);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sign Language:");

  // Camera config (AI-Thinker module)
  camera_config_t config;
  config.ledc_channel = 0;
  config.ledc_timer = 0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_360X480; // 360p
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    lcd.setCursor(0, 1);
    lcd.print("Cam init failed");
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  lcd.setCursor(0, 1);
  lcd.print("WiFi connected   ");
}

void loop() {
  if (millis() - lastSend > 200) { // 200ms = 5 FPS
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      lcd.setCursor(0, 1);
      lcd.print("Capture failed  ");
      return;
    }

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "image/jpeg");

      int httpResponseCode = http.POST(fb->buf, fb->len);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server response: " + response);

        // Parse alphabet from JSON response with better error handling
        String alphabet = "";
        float confidence = 0.0;
        
        // Extract alphabet
        int start = response.indexOf("\"alphabet\":\"") + 12;
        int end = response.indexOf("\"", start);
        if (start > 11 && end > start) {
          alphabet = response.substring(start, end);
        }
        
        // Extract confidence
        int confStart = response.indexOf("\"confidence\":") + 13;
        int confEnd = response.indexOf(",", confStart);
        if (confEnd == -1) confEnd = response.indexOf("}", confStart);
        if (confStart > 12 && confEnd > confStart) {
          confidence = response.substring(confStart, confEnd).toFloat();
        }

        // Print detailed information to Serial Monitor
        Serial.println("=== SIGN LANGUAGE DETECTION ===");
        Serial.println("Recognized Alphabet: " + alphabet);
        Serial.println("Confidence: " + String(confidence, 3));
        Serial.println("================================");

        // Display on LCD with proper formatting
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sign Language:");
        lcd.setCursor(0, 1);
        
        // Handle special characters and long text
        if (alphabet == "nothing") {
          lcd.print("No sign detected");
        } else if (alphabet == "del") {
          lcd.print("Delete");
        } else if (alphabet == "space") {
          lcd.print("Space");
        } else if (alphabet.length() > 0) {
          lcd.print("Alpha: " + alphabet);
        } else {
          lcd.print("No detection");
        }
      } else {
        Serial.printf("HTTP Error code: %d\n", httpResponseCode);
        lcd.setCursor(0, 1);
        lcd.print("HTTP error      ");
      }
      http.end();
    } else {
      Serial.println("WiFi not connected");
      lcd.setCursor(0, 1);
      lcd.print("WiFi lost       ");
    }

    esp_camera_fb_return(fb);
    lastSend = millis();
  }
}