#include "led-manager.h"

LedManager::LedManager(int pinEsp, int pinRed, int pinGreen) {
  _pinEsp = pinEsp;
  _pinRed = pinRed;
  _pinGreen = pinGreen;
  _currentState = STATE_IDLE;
  _lastMillis = 0;
  _toggleState = false;
  _step = 0;
}

void LedManager::begin() {
  pinMode(_pinEsp, OUTPUT);
  pinMode(_pinRed, OUTPUT);
  pinMode(_pinGreen, OUTPUT);
  // ปิดไฟทั้งหมดก่อน
  digitalWrite(_pinEsp, LOW);
  digitalWrite(_pinRed, LOW);
  digitalWrite(_pinGreen, LOW);
}

void LedManager::setMode(LedState state) {
  if (_currentState != state) { // เปลี่ยนเฉพาะเมื่อ mode ไม่เหมือนเดิม
    _currentState = state;
    _step = 0;
    _toggleState = false;
    
    // Reset ไฟทั้งหมดเมื่อเปลี่ยนโหมด กันไฟค้าง
    digitalWrite(_pinEsp, LOW);
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, LOW);
  }
}

void LedManager::update() {
  unsigned long currentMillis = millis();

  switch (_currentState) {
    
    case STATE_WAITING:
      // Logic: กระพริบ LED_ESP ทุก 200ms
      if (currentMillis - _lastMillis >= 200) {
        _lastMillis = currentMillis;
        _toggleState = !_toggleState;
        digitalWrite(_pinEsp, _toggleState ? HIGH : LOW);
      }
      break;

    case STATE_CONNECTED:
      // Logic: เปิด LED_ESP ค้างไว้ตลอด
      digitalWrite(_pinEsp, HIGH);
      digitalWrite(_pinRed, LOW);
      digitalWrite(_pinGreen, LOW);
      break;

    case STATE_ERROR:
      // Logic: Pattern แดงสลับเขียว (เปลี่ยนทุก 200ms)
      if (currentMillis - _lastMillis >= 200) {
        _lastMillis = currentMillis;
        _step++;
        
        if (_step == 1) {
             digitalWrite(_pinRed, HIGH); 
             digitalWrite(_pinGreen, LOW);
        } else if (_step == 2) {
             digitalWrite(_pinRed, LOW); 
        } else if (_step == 3) {
             digitalWrite(_pinGreen, HIGH);
        } else if (_step == 4) {
             digitalWrite(_pinGreen, LOW);
             _step = 0; // วนกลับไปเริ่มใหม่
        }
      }
      break;

    case STATE_IDLE:
    default:
      // ปิดหมด
      digitalWrite(_pinEsp, LOW);
      digitalWrite(_pinRed, LOW);
      digitalWrite(_pinGreen, LOW);
      break;
  }
}