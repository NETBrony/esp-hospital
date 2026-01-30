#include <Arduino.h>
// #include <Wire.h>
// #include <SHT31.h>
#include "wifi_manage.h"

// =========================================================
// USER CONFIGURATION
// =========================================================
const char* SERIAL_NUMBER = "";
const char* SECRET_TOKEN  = "";
const char* DEVICE_NAME   = "";

// =========================================================
// 📦 OBJECTS
// =========================================================
WiFiManager wifiManager;

// ตัวแปรสำหรับเช็คสถานะการเชื่อมต่อ (เพื่อไม่ให้ Serial พิมพ์รัวๆ)
bool isConnectedLog = false;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n-------------------------------------");
  Serial.println("   ESP32 SMART BASE (BLE PROVISION)   ");
  Serial.println("-------------------------------------");
  Serial.printf("Device Serial: %s\n", SERIAL_NUMBER);

  // เริ่มต้นระบบ WiFi / BLE
  // ส่งชื่ออุปกรณ์ไปให้ Class จัดการ เพื่อใช้เป็นชื่อ Bluetooth
  wifiManager.begin(DEVICE_NAME);
}

void loop() {
  // ให้ WiFi Manager ทำงานเบื้องหลัง (เช่น จัดการ BLE)
  wifiManager.loop();

  // ตรวจสอบสถานะการเชื่อมต่อ
  if (wifiManager.isConnected()) {
    if (!isConnectedLog) {
      Serial.println("\n✅ WiFi Connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.printf("Ready to connect Server with Token: %s\n", SECRET_TOKEN);
      isConnectedLog = true;
    }

    // --- พื้นที่สำหรับเขียนโปรแกรมของคุณ (YOUR CODE HERE) ---
    // เช่น อ่าน Sensor, ส่ง MQTT, ฯลฯ
    // mqtt.connect(SERIAL_NUMBER, SECRET_TOKEN);
    
  } else {
    isConnectedLog = false;
    // กรณีหลุด หรือกำลังรอการตั้งค่า
    // Serial.println("Waiting for WiFi...");
    delay(500);
  }
}