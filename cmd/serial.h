#ifndef SERIAL_H
#define SERIAL_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../lib/io.h"
#include "../lib/wifi_config.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppSerial {
private:
  String statusMsg = "READY";

  // netsh コマンドの解析処理
  void parseNetsh(String input) {
    input.trim();
    if (input.length() == 0) return;

    // 小文字統一で判定しやすくする（引数のクォート内は保持したいので前方一致で判定）
    String lower = input;
    lower.toLowerCase();

    // 1. netsh wlan set profile ssid="..." key="..."
    if (lower.startsWith("netsh wlan set profile")) {
      int ssidIdx = input.indexOf("ssid=\"");
      int keyIdx  = input.indexOf("key=\"");

      if (ssidIdx != -1 && keyIdx != -1) {
        int ssidEnd = input.indexOf("\"", ssidIdx + 6);
        int keyEnd  = input.indexOf("\"", keyIdx + 5);

        if (ssidEnd != -1 && keyEnd != -1) {
          String ssid = input.substring(ssidIdx + 6, ssidEnd);
          String pass = input.substring(keyIdx + 5, keyEnd);

          WifiConfig::save(ssid.c_str(), pass.c_str());

          Serial.println("[OK] Wi-Fi Profile Saved to EEPROM!");
          Serial.print("SSID: "); Serial.println(ssid);
          statusMsg = "SAVED: " + ssid;
          return;
        }
      }
      Serial.println("[ERROR] Syntax error. Use: netsh wlan set profile ssid=\"SSID\" key=\"PASS\"");
      statusMsg = "SYNTAX ERROR";
    }
    // 2. netsh wlan show profile
    else if (lower.startsWith("netsh wlan show profile")) {
      WifiCredentials creds = WifiConfig::load();
      if (creds.valid) {
        Serial.println("--- Saved Wi-Fi Profile ---");
        Serial.print("SSID : "); Serial.println(creds.ssid);
        Serial.println("Key  : ******** (Hidden)");
        statusMsg = "SHOW PROFILE";
      } else {
        Serial.println("[INFO] No Wi-Fi profile found in EEPROM.");
        statusMsg = "NO PROFILE";
      }
    }
    // 3. netsh wlan delete profile
    else if (lower.startsWith("netsh wlan delete profile")) {
      WifiConfig::clear();
      Serial.println("[OK] Wi-Fi Profile Deleted.");
      statusMsg = "DELETED";
    }
    else {
      Serial.println("[ERROR] Unknown command in SERIAL mode.");
      statusMsg = "UNKNOWN CMD";
    }
  }

public:
  void init() {
    statusMsg = "WAITING PC...";
    Serial.println("\n==========================================");
    Serial.println(" ArduinoOS PC Communication Mode Started  ");
    Serial.println(" Commands:");
    Serial.println("   netsh wlan set profile ssid=\"SSID\" key=\"PASS\"");
    Serial.println("   netsh wlan show profile");
    Serial.println("   netsh wlan delete profile");
    Serial.println("==========================================\n");
  }

  void onClick() {}
  void onHold() {}

  void update() {
    // シリアル入力のチェック
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      parseNetsh(input);
    }

    // OLED画面描画
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    u8g2.drawStr(0, 10, "SERIAL LINK MODE");
    u8g2.drawHLine(0, 13, 128);

    u8g2.drawStr(0, 30, "STATUS:");
    u8g2.setCursor(0, 42);
    u8g2.print(statusMsg);

    u8g2.drawStr(0, 58, "[PIN 7: EXIT]");
    u8g2.sendBuffer();
  }
};

#endif
