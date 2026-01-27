#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

enum LedState{
  STATE_IDLE,
  STATE_WAITING,
  STATE_CONNECTED,
  STATE_ERROR  
};

class LedManager{
    private:
        int _pinEsp, _pinRed, _pinGreen;
        LedState _currentState;
        unsigned long _lastMillis;
        bool _toggleState;
        int _step;

    public:
        LedManager(int pinEsp, int pinRed, int pinGreen);

    void begin();
    void setMode(LedState state);
    void update();
};

#endif