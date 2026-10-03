#ifndef MORSE_H
#define MORSE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppMorse {
private:
  int brightness = 128;
  String currentMorse = "";
  String decodedText = "";

  unsigned long lastInputTime = 0;
  const unsigned long WORD_TIMEOUT = 1000;  // 1.0秒入力なしで文字確定
  const unsigned long CLEAR_TIMEOUT = 5000; // 5.0秒無操作で自動クリア

  bool wasPressed = false;
  unsigned long pressStart = 0;

  struct MorseMap {
    const char* code;
    char letter;
  };

  const MorseMap morseTable[36] = {
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
    {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
    {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'},
    {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
    {"---..", '8'}, {"----.", '9'}
  };

  char decodeMorse(String code) {
    for (int i = 0; i < 36; i++) {
      if (code == morseTable[i].code) {
        return morseTable[i].letter;
      }
    }
    return '?';
  }

  void resolveMorse() {
    if (currentMorse.length() > 0) {
      char c = decodeMorse(currentMorse);
      decodedText += c;
      currentMorse = "";
    }
  }

public:
  void init() {
    io.setLED(0);
    currentMorse = "";
    decodedText = "";
    brightness = 128;
    wasPressed = false;
    lastInputTime = millis();
  }

  void onClick() {}

  void onHold() {}

  void update() {
    // 1. ロータリーエンコーダー（輝度変更）
    int dir = io.readEncoder();
    if (dir != 0) {
      brightness += dir * 15;
      brightness = constrain(brightness, 5, 255);
      lastInputTime = millis(); // エンコーダー操作でもクリアタイマーをリセット
    }

    // 2. ボタン入力 ＆ リアルタイムLED発光
    bool isPressed = (digitalRead(PIN_SW) == LOW);

    if (isPressed) {
      io.setLED(brightness); // 押した瞬間に即点灯！
      if (!wasPressed) {
        pressStart = millis();
        wasPressed = true;
      }
    } else {
      io.setLED(0); // 離したら即消灯！
      if (wasPressed) {
        unsigned long duration = millis() - pressStart;
        wasPressed = false;

        // 判定しきい値（150ms未満＝短点 '.' / 150ms以上＝長点 '-'）
        if (duration > 10) { // ノイズ除去
          if (duration >= 150) {
            currentMorse += "-";
          } else {
            currentMorse += ".";
          }
          lastInputTime = millis(); // 最終操作時間を更新
        }
      }
    }

    // 3. 自動文字確定（1秒無入力）
    if (currentMorse.length() > 0 && (millis() - lastInputTime > WORD_TIMEOUT)) {
      resolveMorse();
    }

    // 4. 自動画面クリア（5秒無操作）
    if ((currentMorse.length() > 0 || decodedText.length() > 0) && (millis() - lastInputTime > CLEAR_TIMEOUT)) {
      currentMorse = "";
      decodedText = "";
    }

    // 5. OLED描画
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    u8g2.drawStr(0, 10, "MORSE CODE EXE");
    u8g2.drawHLine(0, 13, 128);

    u8g2.setCursor(0, 25);
    u8g2.print("LED BRIGHT: ");
    u8g2.print(brightness);

    u8g2.setCursor(0, 40);
    u8g2.print("CODE: ");
    u8g2.print(currentMorse);

    u8g2.setCursor(0, 55);
    u8g2.print("TEXT: ");
    u8g2.print(decodedText);

    u8g2.sendBuffer();
  }
};

#endif
