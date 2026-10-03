#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROMのアドレスマップ
#define EEPROM_MAGIC_ADDR 100 // 設定済みフラグ (0x41なら設定あり)
#define EEPROM_SSID_ADDR  101 // SSID保存領域 (最大32文字)
#define EEPROM_PASS_ADDR  134 // PASS保存領域 (最大64文字)

struct WifiCredentials {
  char ssid[33];
  char pass[65];
  bool valid;
};

class WifiConfig {
public:
  // EEPROMから設定を読み込む
  static WifiCredentials load() {
    WifiCredentials creds;
    memset(&creds, 0, sizeof(creds));

    uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
    if (magic == 0x41) { // 'A' が入っていれば有効データ
      EEPROM.get(EEPROM_SSID_ADDR, creds.ssid);
      EEPROM.get(EEPROM_PASS_ADDR, creds.pass);
      creds.valid = true;
    } else {
      creds.valid = false;
    }
    return creds;
  }

  // EEPROMに設定を保存する
  static void save(const char* ssid, const char* pass) {
    WifiCredentials creds;
    memset(&creds, 0, sizeof(creds));
    strncpy(creds.ssid, ssid, 32);
    strncpy(creds.pass, pass, 64);

    EEPROM.put(EEPROM_SSID_ADDR, creds.ssid);
    EEPROM.put(EEPROM_PASS_ADDR, creds.pass);
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x41); // 有効フラグ書き込み
  }

  // 設定を消去する
  static void clear() {
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
  }
};

#endif
