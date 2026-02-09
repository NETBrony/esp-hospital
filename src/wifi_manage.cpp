#include "wifi_manage.h"

// ==========================================
// 1. BLE SERVER CALLBACKS
// ==========================================
class MyServerCallbacks: public BLEServerCallbacks {
    WiFiManager* _manager;
public:
    MyServerCallbacks(WiFiManager* manager) { _manager = manager; }

    void onConnect(BLEServer* pServer) {
      Serial.println("[BLE] Mobile App Connected");
      _manager->setDeviceConnected(true);
      
      // เมื่อต่อติด ให้เริ่มสแกน WiFi ทันที (หรือจะรอคำสั่ง SCAN ก็ได้)
      _manager->triggerScan(); 
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("[BLE] Mobile App Disconnected");
      _manager->setDeviceConnected(false);
      
      // เริ่มโฆษณาใหม่ทันที เพื่อให้ต่อใหม่ได้ถ้าหลุด
      BLEDevice::startAdvertising(); 
    }
};

// ==========================================
// 2. BLE WRITE CALLBACKS (รับรหัสผ่าน / คำสั่ง)
// ==========================================
class CredentialsCallbacks: public BLECharacteristicCallbacks {
    WiFiManager* _manager;
public:
    CredentialsCallbacks(WiFiManager* manager) { _manager = manager; }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        String data = String(value.c_str());
        Serial.print("[BLE] Received: ");
        Serial.println(data);

        // ✅ รองรับคำสั่งสแกนใหม่จากหน้าเว็บ
        if (data == "SCAN") {
            Serial.println("[BLE] Command: Rescan requested");
            _manager->triggerScan();
            return;
        }

        // ถ้าไม่ใช่คำสั่ง SCAN ให้พยายามแปลงเป็น JSON (SSID/Pass)
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);

        if (!error) {
            if (doc["ssid"].is<String>() && doc["pass"].is<String>()) {
                String ssid = doc["ssid"].as<String>();
                String pass = doc["pass"].as<String>();
                _manager->connectToWiFi(ssid, pass);
            }
        } else {
            Serial.println("[BLE] JSON Parse Error (Not a config or scan command)");
        }
      }
    }
};

// ==========================================
// 3. WIFI MANAGER IMPLEMENTATION
// ==========================================

WiFiManager::WiFiManager() {}

void WiFiManager::begin(const char* serialNumber) {
    // ⚠️ แก้ไขจุดตาย: ชื่อ Bluetooth ห้ามยาวเกิน 29 ตัวอักษร
    // UUID ปกติยาว 36 ตัว เราจะตัดเอาแค่ 12 ตัวท้าย
    String fullUUID = String(serialNumber);
    String shortId = "";
    
    if (fullUUID.length() > 12) {
        shortId = fullUUID.substring(fullUUID.length() - 12); // เอา 12 ตัวท้าย
    } else {
        shortId = fullUUID;
    }
    
    _deviceName = "ESP32-" + shortId; // ผลลัพธ์: ESP32-edbadeb207de (ยาว 18 ตัว -> ผ่านฉลุย)
    
    // ดึงค่าเก่าจาก Memory
    preferences.begin("wifi-config", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    // ตั้งค่า Mode เป็น Station ก่อนเสมอ
    WiFi.mode(WIFI_STA);

    if (ssid != "") {
        Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
    }

    // ถ้าต่อไม่ติด ให้เปิด BLE
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi not connected. Starting BLE Provisioning...");
        setupBLE();
    } else {
        Serial.println("\n✅ WiFi Connected from saved config.");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
}

void WiFiManager::setupBLE() {
    // เคลียร์ WiFi เก่าก่อนเริ่ม BLE เพื่อความชัวร์
    WiFi.disconnect();
    delay(100);

    BLEDevice::init(_deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(this));

    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Characteristic รับข้อมูล (Write)
    pCharCredentials = pService->createCharacteristic(
        CREDENTIALS_UUID, 
        BLECharacteristic::PROPERTY_WRITE
    );
    pCharCredentials->setCallbacks(new CredentialsCallbacks(this));

    // Characteristic ส่งข้อมูล (Notify)
    pCharWifiList = pService->createCharacteristic(
        WIFI_LIST_UUID, 
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharWifiList->addDescriptor(new BLE2902()); 

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    BLEDevice::startAdvertising();
    
    Serial.printf("[BLE] Device '%s' is advertising...\n", _deviceName.c_str());
}

void WiFiManager::loop() {
    // ทำงาน Scan WiFi ใน Loop หลัก (Non-blocking logic)
    if (_shouldScan && _deviceConnected) {
        _shouldScan = false;
        scanAndSendWiFi();
    }
}

void WiFiManager::scanAndSendWiFi() {
    Serial.println("Scanning WiFi Networks...");
    
    // ✅ เทคนิคสำคัญ: ต้อง Disconnect ก่อน Scan ในบางกรณีถึงจะเจอ
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    Serial.printf("Scan done. Found %d networks.\n", n);

    // ส่งสัญญาณบอก App ให้ล้างลิสต์เก่า (Optional)
    if (pCharWifiList) {
        pCharWifiList->setValue("CLEAR");
        pCharWifiList->notify();
        delay(50);
    }

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    
    // เอาแค่ 10 อันดับแรก
    for (int i = 0; i < n && i < 10; ++i) {
        String ssid = WiFi.SSID(i);
        if(ssid.length() > 0) {
             array.add(ssid);
        }
    }
    
    // แปลงเป็น JSON String
    String output;
    serializeJson(doc, output);
    
    // ส่งกลับไปที่ React
    if (pCharWifiList) {
        // หมายเหตุ: ถ้า JSON ยาวเกิน 512 bytes อาจต้องแบ่งส่ง (Chunk)
        // แต่ถ้าเอาแค่ชื่อ WiFi สั้นๆ 10 ชื่อ น่าจะพอดีกับ MTU ที่ขยายแล้ว หรือส่งทีละชื่อก็ได้
        pCharWifiList->setValue(output.c_str());
        pCharWifiList->notify();
        Serial.println("Sent WiFi List to App");
    }
    
    // ล้างค่า Scan ทิ้งเพื่อประหยัด RAM
    WiFi.scanDelete();
}

void WiFiManager::connectToWiFi(String ssid, String pass) {
    Serial.printf("Connecting to new WiFi: %s\n", ssid.c_str());
    
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);

    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi Connected Successfully!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        // ถ้าต้องการปิด BLE เมื่อต่อติด
        // delay(2000);
        // BLEDevice::deinit(true);
    } else {
        Serial.println("❌ WiFi Connect Failed. Incorrect Password?");
    }
}

void WiFiManager::setDeviceConnected(bool connected) {
    _deviceConnected = connected;
}

void WiFiManager::triggerScan() {
    _shouldScan = true;
}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::resetSettings() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi Settings Cleared!");
}