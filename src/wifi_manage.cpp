#include "wifi_manage.h"

// 1. Constructor
WiFiManager::WiFiManager() : server(80) {}

// 2. Begin Function
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

// 3. Setup AP
void WiFiManager::setupAP(const char* ssid, const char* pass) {
    _isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass);
    
    Serial.print("AP Started. IP: ");
    Serial.println(WiFi.softAPIP());

    // Setup DNS Server for Captive Portal
    dnsServer.start(53, "*", WiFi.softAPIP());

    setupRoutes(); // เรียกฟังก์ชัน setupRoutes ที่เราเขียน
    server.begin();
}

// 4. Setup Routes (ส่วนที่คุณปรับแก้มา ผมรวมให้แล้วครับ)
void WiFiManager::setupRoutes() {
    // ตั้งค่า CORS Header
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // จัดการ Pre-flight Request (OPTIONS)
    server.on("/scan", HTTP_OPTIONS, [](AsyncWebServerRequest *request){ request->send(200); });
    server.on("/connect", HTTP_OPTIONS, [](AsyncWebServerRequest *request){ request->send(200); });

    // API: Scan WiFi
    server.on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request){
        String json = getScanJson();
        request->send(200, "application/json", json);
    });

    // API: รับค่า Connect
    server.on("/connect", HTTP_POST, [&](AsyncWebServerRequest *request){
        String n_ssid = "", n_pass = "";
        
        if (request->hasParam("ssid", true)) n_ssid = request->getParam("ssid", true)->value();
        if (request->hasParam("pass", true)) n_pass = request->getParam("pass", true)->value();

        if (n_ssid != "") {
            preferences.putString("ssid", n_ssid);
            preferences.putString("pass", n_pass);
            
            // ส่ง JSON ตอบกลับ
            request->send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Saved. Restarting...\"}");
            
            // ตั้งค่า Flag เพื่อเตรียม Restart ใน loop
            _shouldRestart = true;
            _restartTimer = millis(); 
            
        } else {
            request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing SSID\"}");
        }
    });

    // Fallback Handler
    server.onNotFound([](AsyncWebServerRequest *request){
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(200, "text/plain", "ESP32 API Ready.");
        }
    });
}

// 5. Helper: Create JSON for Scan
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

// 6. Loop Function (จัดการ Restart ตรงนี้)
void WiFiManager::loop() {
    if (_isAPMode) {
        dnsServer.processNextRequest();
    }

    if (_shouldRestart) {
        // รอ 2 วินาทีเพื่อให้ Response ส่งออกไปจนเสร็จ
        if (millis() - _restartTimer > 2000) { 
            Serial.println("Restarting system...");
            ESP.restart();
        }
    }
}

// 7. Check Connection (ฟังก์ชันนี้แหละที่ขาดไปจนเกิด Linker Error!)
bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

// 8. Reset Settings
void WiFiManager::resetSettings() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi Settings Cleared!");
}