#include "wifi_manage.h"

WiFiManager::WiFiManager() : server(80) {}

void WiFiManager::begin(const char* apName, const char* apPass) {
    preferences.begin("wifi-config", false);
    
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    if (ssid != "") {
        Serial.println("Connecting to saved WiFi: " + ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        // ลองเชื่อมต่อ 10 วินาที
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
            return; // เชื่อมต่อสำเร็จ จบการทำงาน
        } else {
            Serial.println("\nConnection failed. Starting AP Mode.");
        }
    }

    // ถ้าไม่มี WiFi หรือเชื่อมต่อไม่ได้ ให้เปิด Hotspot
    setupAP(apName, apPass);
}

void WiFiManager::setupAP(const char* ssid, const char* pass) {
    _isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass);
    
    Serial.print("AP Started. IP: ");
    Serial.println(WiFi.softAPIP());

    // Setup DNS Server for Captive Portal (redirect all to this IP)
    dnsServer.start(53, "*", WiFi.softAPIP());

    setupRoutes();
    server.begin();
}

void WiFiManager::setupRoutes() {
    // 1. หน้าเว็บหลัก UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", INDEX_HTML);
    });

    // 2. API: Scan WiFi และส่งกลับเป็น JSON
    server.on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request){
        String json = getScanJson();
        request->send(200, "application/json", json);
    });

    // 3. API: รับค่า SSID/Pass จากหน้าเว็บ
    server.on("/connect", HTTP_POST, [&](AsyncWebServerRequest *request){
        String n_ssid = "", n_pass = "";
        if (request->hasParam("ssid", true)) n_ssid = request->getParam("ssid", true)->value();
        if (request->hasParam("pass", true)) n_pass = request->getParam("pass", true)->value();

        if (n_ssid != "") {
            preferences.putString("ssid", n_ssid);
            preferences.putString("pass", n_pass);
            request->send(200, "text/plain", "Saved. Restarting...");
            delay(1000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "Missing SSID");
        }
    });

    // 4. Captive Portal Handler (สำหรับ Android/iOS ที่เช็คเน็ต)
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", INDEX_HTML);
    });
}

String WiFiManager::getScanJson() {
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
        JsonObject obj = array.add<JsonObject>();
        obj["ssid"] = WiFi.SSID(i);
        obj["rssi"] = WiFi.RSSI(i);
        obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void WiFiManager::loop() {
    if (_isAPMode) {
        dnsServer.processNextRequest();
    }
}

void WiFiManager::resetSettings() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi Settings Cleared!");
}