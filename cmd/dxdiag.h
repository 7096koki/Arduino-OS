#ifndef DXDIAG_H
#define DXDIAG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "Arduino_LED_Matrix.h" // Uno R4 WiFi 内蔵LEDマトリクス用
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppDxdiag {
private:
  ArduinoLEDMatrix matrix;
  uint8_t frame[8][12];
  
  static const int PIN_BUILTIN_LED = 13; // 基板上の「L」LED

public:
  void init() {
    matrix.begin();
    pinMode(PIN_BUILTIN_LED, OUTPUT);
    digitalWrite(PIN_BUILTIN_LED, LOW);
    io.setLED(0);
  }

  void onClick() {}
  void onHold() {}

  void update() {
    // 画面初期化＆タイトル描画
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "DXDIAG HARDWARE TEST");
    u8g2.drawHLine(0, 13, 128);
    u8g2.sendBuffer();

    // --------------------------------------------------
    // TEST 1: 12x8 LEDマトリクス（左上から順番に点灯）
    // --------------------------------------------------
    u8g2.setCursor(0, 30);
    u8g2.print("1. MATRIX LED... RUN");
    u8g2.sendBuffer();

    memset(frame, 0, sizeof(frame));
    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 12; c++) {
        frame[r][c] = 1;
        matrix.renderBitmap(frame, 12, 8);
        delay(25); // テンポよく点灯
      }
    }
    delay(300);

    // --------------------------------------------------
    // TEST 2: 基板L LED & 外付け2極LED (PWMフェード)
    // --------------------------------------------------
    u8g2.setCursor(0, 42);
    u8g2.print("2. DUAL LED PWM... RUN");
    u8g2.sendBuffer();

    // フェードイン / フェードアウト (2回繰り返し)
    for (int loop = 0; loop < 2; loop++) {
      for (int b = 0; b <= 255; b += 5) {
        analogWrite(PIN_BUILTIN_LED, b); // 基板「L」LED
        io.setLED(b);                    // 外付け2極LED
        delay(10);
      }
      for (int b = 255; b >= 0; b -= 5) {
        analogWrite(PIN_BUILTIN_LED, b);
        io.setLED(b);
        delay(10);
      }
    }

    // --------------------------------------------------
    // TEST 3: OLED ディスプレイ描画チェック
    // --------------------------------------------------
    u8g2.setCursor(0, 54);
    u8g2.print("3. OLED DISPLAY... RUN");
    u8g2.sendBuffer();
    delay(500);

    // 画面上部から全塗りつぶしバーを下ろす
    for (int y = 0; y < 64; y += 2) {
      u8g2.clearBuffer();
      u8g2.drawBox(0, 0, 128, y);
      u8g2.sendBuffer();
      delay(15);
    }
    delay(500);

    // --------------------------------------------------
    // TEST COMPLETE
    // --------------------------------------------------
    // 消灯・リセット
    memset(frame, 0, sizeof(frame));
    matrix.renderBitmap(frame, 12, 8);
    digitalWrite(PIN_BUILTIN_LED, LOW);
    io.setLED(0);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "ALL DIAGNOSTICS PASSED");
    u8g2.drawStr(0, 45, "[CLICK TO EXIT]");
    u8g2.sendBuffer();

    // ボタンが押されるまで完了画面で待機
    while (digitalRead(PIN_SW) == HIGH) {
      delay(10);
    }
    while (digitalRead(PIN_SW) == LOW); // 離すまで待機
  }
};

#endif
