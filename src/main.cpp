#include <Arduino.h>
#include "wifi_manage.h"

//=====device list========
#define led_esp   = 2;
#define led_red   = 32;
#define led_green = 33;

// =========================================================
// ⚙️ USER CONFIGURATION
// =========================================================
// นำค่าที่ได้จาก React App มาใส่ที่นี่
const char* SERIAL_NUMBER = "ESP32-XXX-001";
const char* SECRET_TOKEN  = "YOUR_SECRET_TOKEN";

WiFiManager wifiManager;

// ตัวแปรเช็คสถานะเพื่อป้องกันการพ่น Log รัวๆ
bool isConnectedLog = false;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n-------------------------------------");
  Serial.println("   ESP32 SMART BASE (BLE PROVISION)   ");
  Serial.println("-------------------------------------");
  
  // ตรวจสอบว่า User กรอกค่ามาหรือยัง
  if (String(SERIAL_NUMBER) == "" || String(SERIAL_NUMBER) == "ESP32-XXX-001") {
    Serial.println("⚠️ WARNING: Please configure SERIAL_NUMBER in main.cpp");
  }

  // ส่ง Serial Number เข้าไป เพื่อใช้เป็นชื่อ Bluetooth
  // User จะเห็นชื่อ Bluetooth ตาม Serial Number ที่ตั้งไว้
  wifiManager.begin(SERIAL_NUMBER);
}

void loop() {
  // ให้ WiFi Manager ทำงานเบื้องหลัง
  wifiManager.loop();

  // ตรวจสอบสถานะการเชื่อมต่อ
  if (wifiManager.isConnected()) {
    
    // ทำงานครั้งเดียวเมื่อต่อเน็ตติด
    if (!isConnectedLog) {
      Serial.println("\n✅ WiFi Connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.printf("Device Ready! Token: %s\n", SECRET_TOKEN);
      isConnectedLog = true;
    }

    // =========================================================
    // 🟢 YOUR MAIN CODE HERE (พื้นที่เขียนโปรแกรมของ User)
    // =========================================================
    // เช่นอ่านค่า Sensor, ส่ง MQTT
    // mqtt.connect(SERIAL_NUMBER, SECRET_TOKEN);
    
  } else {
    // กรณีหลุด หรือกำลังรอการตั้งค่าผ่าน Bluetooth
    isConnectedLog = false;
    
    // ไฟกระพริบ หรือ Logic อื่นๆ ตอนเน็ตหลุด
    delay(200);
  }
}