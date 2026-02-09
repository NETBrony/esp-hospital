#include "ota.h"

OTAManager::OTAManager(String currentVersion, String jsonURL) {
    _currentVersion = currentVersion;
    _jsonURL = jsonURL;
}

void OTAManager::setCallback(OTAStatusCallback callback) {
    _onStatusChange = callback;
}

String OTAManager::getVersion() {
    return _currentVersion;
}

bool OTAManager::checkAndUpdate() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_onStatusChange) _onStatusChange("WiFi Not Connected", -1);
        return false;
    }

    if (_onStatusChange) _onStatusChange("Checking for updates...", 0);
    Serial.println("[OTA] Checking for updates at: " + _jsonURL);

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
        // ⚠️ Production Note: เพื่อความปลอดภัยสูงสุดควรใส่ Root CA Certificate
        // แต่สำหรับ Home Server/Self-signed ให้ใช้ setInsecure() ไปก่อน
        client->setInsecure(); 
    }

    HTTPClient http;
    // กำหนด Timeout นานหน่อยเผื่อ Server ตอบช้า
    http.setTimeout(5000); 
    
    if (!http.begin(*client, _jsonURL)) {
        if (_onStatusChange) _onStatusChange("Connect Failed", -1);
        delete client;
        return false;
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("[OTA] Config Found: " + payload);

        // Parse JSON (รองรับ ArduinoJson v6 และ v7)
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            String newVersion = doc["version"].as<String>();
            String binFile = doc["file"].as<String>();
            
            // เปรียบเทียบเวอร์ชัน (Logic แบบง่าย: String Compare)
            // แนะนำ: ถ้าเป็นเลขเวอร์ชันซับซ้อน (1.0.10 vs 1.0.2) อาจต้องเขียนฟังก์ชัน Compare แยก
            if (newVersion > _currentVersion) {
                String msg = "New version found: " + newVersion;
                Serial.println("[OTA] " + msg);
                if (_onStatusChange) _onStatusChange(msg, 10);
                
                // สร้าง Full URL สำหรับไฟล์ .bin (ถ้าใน json ให้มาแค่ชื่อไฟล์)
                // สมมติ URL เดิมคือ https://domain.com/ota/version.json
                // เราจะตัด version.json ออกแล้วเติมชื่อไฟล์ .bin ใส่แทน
                String baseURL = _jsonURL.substring(0, _jsonURL.lastIndexOf('/') + 1);
                String binURL = baseURL + binFile;

                // ถ้าใน JSON ให้ URL เต็มมาแล้วก็ใช้ได้เลย
                if (binFile.startsWith("http")) {
                    binURL = binFile; 
                }

                http.end(); // ปิด Connection JSON ก่อนเริ่มโหลดไฟล์ใหญ่
                delete client; // Clean up ก่อน
                
                return _performUpdate(binURL);
            } else {
                Serial.println("[OTA] Device is up to date.");
                if (_onStatusChange) _onStatusChange("Device is up to date", 100);
            }
        } else {
            Serial.print("[OTA] JSON Parse Failed: ");
            Serial.println(error.c_str());
            if (_onStatusChange) _onStatusChange("JSON Error", -1);
        }
    } else {
        Serial.printf("[OTA] HTTP Error: %d\n", httpCode);
        if (_onStatusChange) _onStatusChange("HTTP Error " + String(httpCode), -1);
    }

    http.end();
    delete client;
    return false;
}

bool OTAManager::_performUpdate(String binURL) {
    if (_onStatusChange) _onStatusChange("Downloading Firmware...", 20);
    Serial.println("[OTA] Downloading from: " + binURL);

    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) client->setInsecure();

    HTTPClient http;
    http.setTimeout(10000); // 10 วินาที Timeout

    if (!http.begin(*client, binURL)) {
        if (_onStatusChange) _onStatusChange("Bin Download Failed", -1);
        delete client;
        return false;
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        Serial.printf("[OTA] File size: %d bytes\n", contentLength);

        if (contentLength <= 0) {
             if (_onStatusChange) _onStatusChange("Content-Length Error", -1);
             return false;
        }

        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
            if (_onStatusChange) _onStatusChange("Flashing...", 50);
            
            // 🚀 Magic Function: Stream ข้อมูลจาก HTTP ลง Flash โดยตรง
            size_t written = Update.writeStream(http.getStream());

            if (written == contentLength) {
                Serial.println("[OTA] Write successful. Finishing...");
                if (Update.end()) {
                    if (Update.isFinished()) {
                        Serial.println("[OTA] Update Success! Rebooting...");
                        if (_onStatusChange) _onStatusChange("Update Success! Rebooting...", 100);
                        delay(1000);
                        ESP.restart(); // รีบูตเครื่อง
                        return true;
                    }
                } else {
                    Serial.printf("[OTA] Update Error: %u\n", Update.getError());
                    if (_onStatusChange) _onStatusChange("Update Error: " + String(Update.getError()), -1);
                }
            } else {
                Serial.printf("[OTA] Write Failed: written %d / expected %d\n", written, contentLength);
                if (_onStatusChange) _onStatusChange("Write Failed", -1);
            }
        } else {
            Serial.println("[OTA] Not enough space to update");
            if (_onStatusChange) _onStatusChange("Not enough space", -1);
        }
    } else {
        Serial.printf("[OTA] Bin HTTP Error: %d\n", httpCode);
        if (_onStatusChange) _onStatusChange("Bin HTTP Error", -1);
    }

    http.end();
    delete client;
    return false;
}