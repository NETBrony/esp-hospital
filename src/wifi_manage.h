#ifndef WIFI_MANAGE_H
#define WIFI_MANAGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ⚠️ UUID ต้องตรงกับ React: DeviceManage.jsx
#define SERVICE_UUID           "0000aaaa-0000-1000-8000-00805f9b34fb"
#define WIFI_LIST_UUID         "0000bbbb-0000-1000-8000-00805f9b34fb" // Notify (ส่งรายชื่อ WiFi)
#define CREDENTIALS_UUID       "0000cccc-0000-1000-8000-00805f9b34fb" // Write (รับรหัสผ่าน)

class WiFiManager {
private:
    String _deviceName;
    Preferences preferences;
    BLEServer* pServer = NULL;
    BLECharacteristic* pCharWifiList = NULL;
    BLECharacteristic* pCharCredentials = NULL;
    
    // สถานะสำหรับ Loop
    bool _deviceConnected = false;
    bool _shouldScan = false;

public:
    WiFiManager();
    
    // เริ่มทำงาน: เช็ค WiFi เก่าก่อน ถ้าไม่มีค่อยเปิด BLE
    void begin(const char* serialNumber);
    
    // ใส่ใน void loop() ของ Main เพื่อคอยตรวจจับเหตุการณ์
    void loop();
    
    // ฟังก์ชันภายใน (แต่ต้อง Public เพื่อให้ Callback เรียกได้)
    void setupBLE();
    void connectToWiFi(String ssid, String pass);
    void scanAndSendWiFi();
    void setDeviceConnected(bool connected);
    void triggerScan(); // สั่งให้เริ่มสแกน (ใช้โดย Callback)

    // Utility
    bool isConnected();
    void resetSettings();
};

#endif