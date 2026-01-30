#ifndef WIFI_MANAGE_H
#define WIFI_MANAGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class WiFiManager {
public:
    WiFiManager();
    // [แก้ไข] รับ Serial Number เพื่อไปตั้งเป็นชื่อ Bluetooth
    void begin(const char* serialNumber); 
    
    void loop();
    bool isConnected();
    void resetSettings();

    // Callback functions
    void connectToWiFi(String ssid, String pass);
    void scanAndSendWiFi();

private:
    Preferences preferences;
    String _deviceName; // เก็บชื่ออุปกรณ์ (Serial Number)

    BLEServer* pServer = NULL;
    BLECharacteristic* pCharCommand = NULL;
    BLECharacteristic* pCharData    = NULL;
    BLECharacteristic* pCharStatus  = NULL;

    void setupBLE();
};

#endif