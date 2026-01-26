#include <Arduino.h>
#include <Wire.h>
#include <SHT31.h>
#include "led-manager.h"
#include "wifi_manage.h" 

// --- Pin Definitions ---
#define LED_ESP 2
#define LED_RED 32
#define LED_GREEN 33
// #define PIN_BOOT 0  <-- ตัดออก: ไม่ได้ใช้แล้ว

// --- Objects ---
LedManager ledManager(LED_ESP, LED_RED, LED_GREEN);
SHT31 sht30 = SHT31();

// ตรวจสอบชื่อ Class ใน wifi_manage.h ให้ตรงกัน (เช่น WiFiManager หรือ NBM_WiFiManager)
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
  
  // เริ่มต้น LED Manager
  ledManager.begin();

  // 2. Init Sensor
  Serial.println("Initializing SHT31...");
  if (!sht30.begin()) {
    Serial.println("SHT31 Error!");
  }

  // -----------------------------------------------------------
  // 3. Start WiFi Manager (หัวใจสำคัญอยู่ตรงนี้)
  // -----------------------------------------------------------
  // ฟังก์ชัน .begin() นี้ (ตาม Library ที่เราเขียนกันก่อนหน้า) มี Logic คือ:
  //  - อ่านค่า WiFi เก่าจาก Memory
  //  - พยายามเชื่อมต่อ WiFi นั้น (รอนานสุด 10 วินาที)
  //  - ถ้า "ต่อติด" -> จบฟังก์ชัน ไปทำงานต่อ (ไฟเขียว)
  //  - ถ้า "ต่อไม่ติด" หรือ "ไม่มีค่าเก่า" -> จะเปิด Hotspot ชื่อ "SmartFarm_Setup" ทันที
  
  Serial.println("Attempting to connect to saved WiFi...");
  wifiManager.begin("SmartFarm_Setup"); 
}

void loop() {
  // --- A. ให้ Web Portal ทำงานตลอดเวลา ---
  // จำเป็นต้องใส่ไว้ เพื่อให้คนเข้ามาตั้งค่าใหม่ได้ กรณีที่มันเด้งเข้า Hotspot Mode
  wifiManager.loop();

  // --- B. ไฟแสดงสถานะ ---
  if (wifiManager.isConnected()) {
    // ✅ สถานะ: Online (ต่อ WiFi สำเร็จ)
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_ESP, HIGH); 
  } else {
    // ⚠️ สถานะ: Setup Mode (กำลังปล่อย Hotspot รอการตั้งค่า)
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    // กระพริบไฟ ESP เบาๆ
    digitalWrite(LED_ESP, (millis() / 500) % 2); 
  }

  // --- C. อ่านค่า Sensor (Non-blocking) ---
  if (millis() - lastSensorRead > interval) {
    lastSensorRead = millis();
    
    float t = sht30.getTemperature();
    float h = sht30.getHumidity();

    if (!isnan(t)) {
      Serial.printf("[Sensor] Temp: %.2f C, Humi: %.2f %%\n", t, h);
    } else {
      Serial.println("[Sensor] Reading Failed");
    }
  }
}