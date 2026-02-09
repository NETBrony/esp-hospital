#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// กำหนดประเภทของ Callback function เพื่อส่งสถานะกลับไปหน้า Main
// (Status Message, Progress 0-100)
typedef std::function<void(String, int)> OTAStatusCallback;

class OTAManager {
private:
    String _currentVersion;
    String _jsonURL;
    OTAStatusCallback _onStatusChange; // ตัวแปรเก็บฟังก์ชัน Callback

    // ฟังก์ชันภายใน: ดาวน์โหลดและ Flash ไฟล์ .bin
    bool _performUpdate(String binURL);

public:
    OTAManager(String currentVersion, String jsonURL);

    // ลงทะเบียนฟังก์ชันที่จะให้เรียกเมื่อสถานะเปลี่ยน (เช่น ส่ง MQTT)
    void setCallback(OTAStatusCallback callback);

    // เช็คเวอร์ชันและทำการอัปเดต (Return true ถ้าอัปเดตสำเร็จและกำลัง Reboot)
    bool checkAndUpdate();
    
    // ดึงเวอร์ชันปัจจุบัน
    String getVersion();
};

#endif