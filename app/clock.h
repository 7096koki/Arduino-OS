#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

// NTP通信用の設定 (JST: +9時間 = 32400秒)
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

class AppClock {
private:
  int year = 2026;
  int month = 9;
  int day = 3;
  int hours = 12;
  int minutes = 0;
  int seconds = 0;
  
  unsigned long lastSecondMillis = 0;
  bool isWifiConnected = false;

  int editState = 0;
  bool holdTriggered = false;

  int getDaysInMonth(int y, int m) {
    if (m == 2) {
      if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) return 29;
      return 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
  }

  void drawAnalogClock(int centerX, int centerY, int radius) {
    u8g2.drawCircle(centerX, centerY, radius);
    u8g2.drawPixel(centerX, centerY);

    float hRad = ((hours % 12) + minutes / 60.0) * (360.0 / 12.0) * DEG_TO_RAD - HALF_PI;
    float mRad = (minutes + seconds / 60.0) * (360.0 / 60.0) * DEG_TO_RAD - HALF_PI;
    float sRad = seconds * (360.0 / 60.0) * DEG_TO_RAD - HALF_PI;

    int hX = centerX + cos(hRad) * (radius - 5);
    int hY = centerY + sin(hRad) * (radius - 5);
    u8g2.drawLine(centerX, centerY, hX, hY);

    int mX = centerX + cos(mRad) * (radius - 2);
    int mY = centerY + sin(mRad) * (radius - 2);
    u8g2.drawLine(centerX, centerY, mX, mY);

    int sX = centerX + cos(sRad) * (radius - 1);
    int sY = centerY + sin(sRad) * (radius - 1);
    u8g2.drawPixel(sX, sY);
  }

public:
  void init() {
    lastSecondMillis = millis();
    editState = 0;
    holdTriggered = false;

    if (WiFi.status() == WL_CONNECTED) {
      isWifiConnected = true;
      timeClient.begin();
      timeClient.update();
    } else {
      isWifiConnected = false;
    }
  }

  void onClick() {
    if (holdTriggered) {
      holdTriggered = false;
      return;
    }

    if (isWifiConnected) return;

    if (editState > 0) {
      editState++;
      if (editState > 5) {
        editState = 0;
        seconds = 0;
      }
    }
  }

  void onHold() {
    if (!isWifiConnected && editState == 0 && !holdTriggered) {
      editState = 1;
      holdTriggered = true;
    }
  }

  void update() {
    if (isWifiConnected) {
      timeClient.update();
      unsigned long epochTime = timeClient.getEpochTime();

      if (epochTime > 100000) {
        // 時分秒の取得
        hours = timeClient.getHours();
        minutes = timeClient.getMinutes();
        seconds = timeClient.getSeconds();

        // エポック秒から正確な年月日を自前計算（文字化け防止策）
        unsigned long rawDays = epochTime / 86400;
        unsigned long days = rawDays;
        
        // 1970年からの経過年を計算
        int y = 1970;
        while (true) {
          bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
          int daysInYear = leap ? 366 : 365;
          if (days >= daysInYear) {
            days -= daysInYear;
            y++;
          } else {
            break;
          }
        }
        year = y;

        // 月と日の計算
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        int daysInMonths[] = {31, (leap ? 29 : 28), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int m = 0;
        while (m < 12 && days >= daysInMonths[m]) {
          days -= daysInMonths[m];
          m++;
        }
        month = m + 1;
        day = days + 1;
      }
    } else { // <-- ここにあった 「};」 を 「}」 に修正
      if (editState == 0 && (millis() - lastSecondMillis >= 1000)) {
        lastSecondMillis = millis();
        seconds++;
        if (seconds >= 60) {
          seconds = 0;
          minutes++;
          if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
              hours = 0;
              day++;
              if (day > getDaysInMonth(year, month)) {
                day = 1;
                month++;
                if (month > 12) {
                  month = 1;
                  year++;
                }
              }
            }
          }
        }
      }

      int dir = io.readEncoder();
      if (dir != 0 && editState > 0) {
        switch (editState) {
          case 1: year = constrain(year + dir, 2000, 2099); break;
          case 2: month = (month + dir - 1 + 12) % 12 + 1; break;
          case 3: {
            int maxDays = getDaysInMonth(year, month);
            day = (day + dir - 1 + maxDays) % maxDays + 1;
            break;
          }
          case 4: hours = (hours + dir + 24) % 24; break;
          case 5: minutes = (minutes + dir + 60) % 60; break;
        }
      }
    }

    u8g2.clearBuffer();

    drawAnalogClock(18, 18, 12);

    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d/%02d/%02d", year, month, day);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(42, 20, dateStr);

    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hours, minutes, seconds);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(18, 46, timeStr);

    u8g2.setFont(u8g2_font_4x6_tr);
    if (isWifiConnected) {
      u8g2.drawStr(28, 62, "[NTP SYNC: AUTO]");
    } else {
      switch (editState) {
        case 1: u8g2.drawStr(26, 62, "[SET YEAR: TURN]"); break;
        case 2: u8g2.drawStr(24, 62, "[SET MONTH: TURN]"); break;
        case 3: u8g2.drawStr(28, 62, "[SET DAY: TURN]"); break;
        case 4: u8g2.drawStr(26, 62, "[SET HOUR: TURN]"); break;
        case 5: u8g2.drawStr(22, 62, "[SET MINUTE: TURN]"); break;
        default: u8g2.drawStr(16, 62, "HOLD SW: MANUAL SET"); break;
      }
    }

    u8g2.sendBuffer();
  }
};

#endif
