#ifndef WIFI_MANAGE_H
#define WIFI_MANAGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

class WiFiManager {
public:
    WiFiManager();
    void begin(const char* apName, const char* apPass = NULL);
    void loop(); // สำคัญ! เราจะสั่ง Restart ในนี้
    bool isConnected();
    void resetSettings();

private:
    AsyncWebServer server;
    DNSServer dnsServer;
    Preferences preferences;
    bool _isAPMode = false;

    // [เพิ่ม] ตัวแปรสำหรับหน่วงเวลา Restart
    bool _shouldRestart = false;
    unsigned long _restartTimer = 0;

    void setupAP(const char* ssid, const char* pass);
    void setupRoutes();
    String getScanJson();
};

#endif