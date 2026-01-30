#include "wifi_manage.h"

// =========================================================
// 🔒 SYSTEM UUIDS (HIDDEN FROM USER)
// =========================================================
// ส่วนนี้ User ทั่วไปไม่ต้องแก้ไข เป็นมาตรฐานของ Platform คุณ
#define SERVICE_UUID           "00000001-5e26-4ac5-9004-76aa55060412"
#define CHAR_COMMAND_UUID      "00000002-5e26-4ac5-9004-76aa55060412"
#define CHAR_DATA_UUID         "00000003-5e26-4ac5-9004-76aa55060412"
#define CHAR_STATUS_UUID       "00000004-5e26-4ac5-9004-76aa55060412"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("[BLE] App Connected");
    };
    void onDisconnect(BLEServer* pServer) {
      Serial.println("[BLE] App Disconnected");
      pServer->getAdvertising()->start(); 
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    WiFiManager* _manager;
public:
    MyCallbacks(WiFiManager* manager) { _manager = manager; }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      String data = String(value.c_str());
      
      if (data.length() > 0) {
        Serial.print("[BLE] Received: ");
        Serial.println(data);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);

        if (!error) {
            // ArduinoJson v7 Syntax
            if (doc["ssid"].is<const char*>() && doc["pass"].is<const char*>()) {
                String ssid = doc["ssid"];
                String pass = doc["pass"];
                _manager->connectToWiFi(ssid, pass);
            }
        } 
        else if (data == "SCAN") {
            _manager->scanAndSendWiFi();
        }
      }
    }
};

WiFiManager::WiFiManager() {}

// [แก้ไข] รับ Serial Number มาตั้งเป็นชื่อ Bluetooth
void WiFiManager::begin(const char* serialNumber) {
    _deviceName = serialNumber; // ใช้ Serial เป็นชื่อ Bluetooth เลย
    
    preferences.begin("wifi-config", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    if (ssid != "") {
        Serial.println("Connecting to saved WiFi...");
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Starting BLE Provisioning...");
        setupBLE();
    }
}

void WiFiManager::setupBLE() {
    // ใช้ _deviceName (Serial Number) เป็นชื่อ Bluetooth
    BLEDevice::init(_deviceName.c_str()); 
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharData = pService->createCharacteristic(CHAR_DATA_UUID, BLECharacteristic::PROPERTY_WRITE);
    pCharData->setCallbacks(new MyCallbacks(this));

    pCharStatus = pService->createCharacteristic(CHAR_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharStatus->addDescriptor(new BLE2902());

    pCharCommand = pService->createCharacteristic(CHAR_COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
    pCharCommand->setCallbacks(new MyCallbacks(this));

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    BLEDevice::startAdvertising();
    
    Serial.printf("[BLE] Device '%s' is ready to connect.\n", _deviceName.c_str());
}

void WiFiManager::connectToWiFi(String ssid, String pass) {
    Serial.printf("Saving Creds: %s / %s\n", ssid.c_str(), pass.c_str());
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);

    if (pCharStatus) pCharStatus->setValue("Connecting...");
    if (pCharStatus) pCharStatus->notify();

    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (pCharStatus) pCharStatus->setValue("Connected");
        if (pCharStatus) pCharStatus->notify();
        Serial.println("WiFi Connected! Stopping BLE.");
    } else {
        if (pCharStatus) pCharStatus->setValue("Failed");
        if (pCharStatus) pCharStatus->notify();
        Serial.println("Connection Failed.");
    }
}

void WiFiManager::scanAndSendWiFi() {
    Serial.println("Scanning WiFi...");
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (int i = 0; i < n && i < 5; ++i) {
        array.add(WiFi.SSID(i));
    }
    
    String output;
    serializeJson(doc, output);
    
    Serial.println(output);
    if (pCharStatus) {
        pCharStatus->setValue(output.c_str());
        pCharStatus->notify();
    }
}

void WiFiManager::loop() {}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::resetSettings() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
}