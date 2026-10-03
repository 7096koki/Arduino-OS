#ifndef IO_H
#define IO_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <RotaryEncoder.h>

const int PIN_A          = 3; // 2から3に変更
const int PIN_B          = 2; // 3から2に変更
const int PIN_SW         = 4;
const int PIN_STATUS_LED = 6; // OS共通LED（PWM対応ピン）

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern RotaryEncoder encoder;

// Wi-Fiアイコンのビットマップデータ
static const unsigned char wifi_none[] = { 0b01110000, 0b01010000, 0b01110000, 0b00100000, 0b00100000, 0b00100000, 0b00100000 };
static const unsigned char wifi_low[]  = { 0b01110000, 0b01010000, 0b01110000, 0b00100100, 0b00100100, 0b00100100, 0b00100100 };
static const unsigned char wifi_full[] = { 0b01110000, 0b01010000, 0b01110000, 0b00100100, 0b00100100, 0b00100101, 0b00100101 };

inline void checkPosition() {
  encoder.tick();
}

class IO {
public:
  void begin() {
    pinMode(PIN_SW, INPUT_PULLUP);
    pinMode(PIN_STATUS_LED, OUTPUT);
    analogWrite(PIN_STATUS_LED, 0); // 初期は消灯

    attachInterrupt(digitalPinToInterrupt(PIN_A), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_B), checkPosition, CHANGE);
  }

  // LEDの輝度制御 (0〜255)
  void setLED(int brightness) {
    analogWrite(PIN_STATUS_LED, constrain(brightness, 0, 255));
  }

  // ON / OFF 簡易制御
  void setLED(bool state) {
    digitalWrite(PIN_STATUS_LED, state ? HIGH : LOW);
  }

  int readEncoder() {
    int dir = (int)encoder.getDirection();
    return dir;
  }

  int getEncDelta() {
    int dir = (int)encoder.getDirection();
    return dir;
  }

  bool isButtonPressed() {
    if (digitalRead(PIN_SW) == LOW) {
      delay(200);
      while (digitalRead(PIN_SW) == LOW);
      return true;
    }
    return false;
  }

  void drawWifiIcon(int x, int y) {
    if (WiFi.status() == WL_CONNECTED) {
      long rssi = WiFi.RSSI();
      if (rssi > -67) {
        u8g2.drawXBM(x, y, 8, 7, wifi_full);
      } else {
        u8g2.drawXBM(x, y, 8, 7, wifi_low);
      }
    } else {
      u8g2.drawXBM(x, y, 8, 7, wifi_none);
    }
  }
};

#endif
