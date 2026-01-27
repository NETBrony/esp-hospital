#include <Arduino.h>
#include <Wire.h>
#include <SHT31.h>
#include "led-manager.h"
#include "wifi_manage.h" 

// --- Pin Definitions ---
#define LED_ESP 2
#define LED_RED 32
#define LED_GREEN 33

// --- Objects ---
LedManager ledManager(LED_ESP, LED_RED, LED_GREEN);

// [แก้ไข 1] ระบุ Address 0x44 (ค่ามาตรฐาน) หรือ 0x45 ให้ชัดเจน
SHT31 sht30(0x44); 

WiFiManager wifiManager; 

// --- Variables ---
unsigned long lastSensorRead = 0;
const long interval = 2000; 

void setup() {
  Serial.begin(115200);
  delay(100);

  // 1. Init LEDs
  pinMode(LED_ESP, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  ledManager.begin();

  // -----------------------------------------------------------
  // [แก้ไข 2] ต้องสั่ง Start I2C Bus ก่อนเสมอ!
  // -----------------------------------------------------------
  // สำหรับ ESP32 มาตรฐาน: SDA=21, SCL=22
  Wire.begin(); 
  // หรือถ้าคุณต่อขาอื่น ให้ใช้: Wire.begin(SDA_PIN, SCL_PIN);

  // 2. Init Sensor
  Serial.println("Initializing SHT31...");
  
  // Library บางตัวต้องการ Wire object ใน begin
  if (sht30.begin()) { 
    Serial.println("SHT31 Found!");
  } else {
    Serial.println("SHT31 Error! Check wiring.");
  }

  // 3. Start WiFi Manager
  Serial.println("Attempting to connect to saved WiFi...");
  wifiManager.begin("SmartFarm_Setup"); 
}

void loop() {
  // --- A. ให้ Web Portal ทำงานตลอดเวลา ---
  wifiManager.loop();

  // --- B. ไฟแสดงสถานะ ---
  if (wifiManager.isConnected()) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_ESP, HIGH); 
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_ESP, (millis() / 500) % 2); 
  }

  // --- C. อ่านค่า Sensor ---
  if (millis() - lastSensorRead > interval) {
    lastSensorRead = millis();
    
    // -----------------------------------------------------------
    // [แก้ไข 3] ต้องสั่ง .read() ก่อนเรียกค่าเสมอ (สำหรับ RobTillaart Lib)
    // -----------------------------------------------------------
    bool success = sht30.read(); 

    if (success) {
      float t = sht30.getTemperature();
      float h = sht30.getHumidity();
      Serial.printf("[Sensor] Temp: %.1f °C, Humi: %.1f %%\n", t, h);
    } else {
      Serial.println("[Sensor] Read Fail! (Check wiring or Pull-up resistors)");
    }
  }
}