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
    void begin(const char* deviceName);
    void loop();
    bool isConnected();
    void resetSettings();

    // [แก้ไข] ย้ายลงมาตรงนี้เพื่อให้ Callback เรียกใช้ได้
    void connectToWiFi(String ssid, String pass);
    void scanAndSendWiFi();

private:
    Preferences preferences;
    bool _isProvisioningMode = false;
    String _deviceName;

    BLEServer* pServer = NULL;
    BLECharacteristic* pCharCommand = NULL;
    BLECharacteristic* pCharData    = NULL;
    BLECharacteristic* pCharStatus  = NULL;

    void setupBLE();
};

#endif