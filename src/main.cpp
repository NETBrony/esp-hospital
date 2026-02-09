#include <Arduino.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "SHT31.h"
#include "wifi_manage.h"
#include "mqtt.h"
#include "ota.h"

//================= SERIAL NUMBER & DEVICE TOKEN ===================
const char* serial_number = "49c4e46a-a15b-4f41-91f3-edbadeb207de";
const char* device_token  = "e5c0326dac99f43373d8c9b1239d5384";
//==================================================================

#define OTA_URL "https://aierpc.dpdns.org/ota/version.json"

//========== FIRMWARE VERSION ================
#define FW_VERSION "1.0.0"
//============================================

OTAManager ota(OTA_URL, FW_VERSION);

#define LED_ESP   2
#define LED_RED   32
#define LED_GREEN 33

#define SHT30_ADDRESS 0x44

WiFiManager wifiManager;

// ✅ แก้ไขการประกาศ SHT31 ให้รับ Address ตรงนี้
SHT31 sht(SHT30_ADDRESS);

// MQTT Objects
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Timing Variables
bool isConnectedLog = false;
unsigned long previousMillis = 0;
const long interval = 500;        
unsigned long lastMsgTime = 0;
const long msgInterval = 5000;   
bool ledState = LOW;

// =========================================================
// 📡 MQTT FUNCTIONS
// =========================================================

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == topic_control) {
    if (message == "ON") {
      Serial.println("Pump turned ON");
    } else if (message == "OFF") {
      Serial.println("Pump turned OFF");
    }
  }

  if (String(topic) == topic_ota_update) {
        if (message == "CHECK") {
             // เรียก OTA ทำงาน (ระวัง: ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าจะโหลดเสร็จ)
             ota.checkAndUpdate();
        }
    }
}

void setupMQTT() {
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void reconnectMQTT() {
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(topic_control);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again later");
    }
  }
}

void onOTAStatus(String msg, int progress) {
    // แสดงใน Serial Monitor
    Serial.printf("[OTA_CB] %s (%d%%)\n", msg.c_str(), progress);
    
    // ส่ง MQTT กลับไปให้ React
    // Format JSON: {"status": "Downloading...", "progress": 20}
    String json = "{\"status\":\"" + msg + "\", \"progress\":" + String(progress) + "}";
    client.publish(topic_ota_status, json.c_str());
}

// =========================================================
// 🌡️ SENSOR FUNCTIONS
// =========================================================

void readAndPublishSensor() {
  sht.read(); 

  float t = sht.getTemperature();
  float h = sht.getHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from SHT30 sensor!");
    return;
  }

  // ✅ เพิ่ม Logic การปัดเศษทศนิยม 2 ตำแหน่ง
  // หลักการ: 36.907 -> *100 = 3690.7 -> round = 3691 -> /100.0 = 36.91
  float t_rounded = round(t * 100.0) / 100.0;
  float h_rounded = round(h * 100.0) / 100.0;

  JsonDocument doc;
  doc["temp"] = t_rounded;
  doc["hum"]  = h_rounded;
  doc["device"] = serial_number; 
  
  char buffer[256];
  serializeJson(doc, buffer);

  if (client.publish(topic_TempHumi, buffer)) {
    Serial.print("Published: ");
    Serial.println(buffer);
  } else {
    Serial.println("Publish failed");
  }
}

// =========================================================
// 🚀 MAIN SETUP & LOOP
// =========================================================

void setup() {
  Serial.begin(115200);
  delay(100);

  // 1. Setup Pins
  pinMode(LED_ESP, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);

  // 2. Setup SHT30
  Wire.begin();
  
  // ✅ แก้ไข: ไม่ต้องส่ง Address ใน begin() แล้ว เพราะใส่ไปตอนประกาศแล้ว
  sht.begin(); 
  
  uint16_t stat = sht.readStatus();
  Serial.print("SHT31 status: ");
  Serial.println(stat, HEX);

  // 3. Setup WiFi Manager
  Serial.println("\n-------------------------------------");
  Serial.println("   ESP32 SMART BASE (BLE PROVISION)   ");
  Serial.println("-------------------------------------");
  
  // ถ้าเพิ่ง Flash ใหม่ มันอาจจะยังจำค่าขยะอยู่ ถ้าอยากล้างค่าให้เปิดบรรทัดนี้ 1 รอบ
  // wifiManager.resetSettings(); 

  wifiManager.begin(serial_number);
  setupMQTT();
  ota.setCallback(onOTAStatus);
  client.publish("status/version", FW_VERSION);

}

void loop() {
  wifiManager.loop();

  unsigned long currentMillis = millis();

  if (wifiManager.isConnected()) {
    
    if (!isConnectedLog) {
      Serial.println("\n✅ WiFi Connected!");
      Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
      isConnectedLog = true;
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_ESP, HIGH);
    }

    if (!client.connected()) {
      static unsigned long lastReconnectAttempt = 0;
      if (currentMillis - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = currentMillis;
        reconnectMQTT();
      }
    } else {
      client.loop();
      if (currentMillis - lastMsgTime >= msgInterval) {
        lastMsgTime = currentMillis;
        readAndPublishSensor();
      }
    }

  } else {
    isConnectedLog = false;
    digitalWrite(LED_GREEN, LOW);

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_RED, ledState);
      digitalWrite(LED_ESP, ledState);
    }
  }
}