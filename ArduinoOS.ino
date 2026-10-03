#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <RotaryEncoder.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.nict.jp", 9 * 3600, 60000); // タイムゾーン+9時間、更新間隔60秒


#include "cmd/serial.h"
#include "cmd/dxdiag.h"

#include "lib/io.h"
#include "lib/wifi_config.h"
#include "app/clock.h"
#include "app/pong.h"
#include "app/tetris.h"
#include "app/invader.h"
#include "app/morse.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
RotaryEncoder encoder(PIN_A, PIN_B, RotaryEncoder::LatchMode::FOUR3);

const int PIN_EXIT_GND = 12;

IO io;
AppClock appClock;
AppPong appPong;
AppTetris appTetris;
AppInvader appInvader;
AppMorse appMorse;
AppDxdiag appDxdiag;
AppSerial appSerial;

template <typename T>
void runApp(T& app);

void runClock();
void runPong();
void runTetris();
void runInvader();
void runMorse();
void runDxdiag();
void runSerial();

struct AppEntry {
  const char* name;
  void (*runner)();
};

void runClock()   { runApp(appClock); }
void runPong()    { runApp(appPong); }
void runTetris()  { runApp(appTetris); }
void runInvader() { runApp(appInvader);}
void runMorse()   { runApp(appMorse);}
void runDxdiag()  { runApp(appDxdiag);}
void runSerial() { runApp(appSerial); }

const AppEntry appTable[] = {
  {"CLOCK.EXE",   runClock},
  {"PONG.EXE",    runPong},
  {"TETRIS.EXE",  runTetris},
  {"INVADER.EXE", runInvader},
  {"MORSE.EXE", runMorse}

};
const int argCount = sizeof(appTable) / sizeof(appTable[0]);

const char* cmdList[] = {"START", "CLEAR", "DXDIAG", "SERIAL"};
const int cmdCount = sizeof(cmdList) / sizeof(cmdList[0]);

int currentWordIdx = 0;
String currentPrompt = "C:\\>";
String lastOutput = "";
bool selectingCommand = true;

template <typename T>
void runApp(T& app) {
  app.init();
  bool running = true;

  while (running) {
    // io.updateLED();

    if (digitalRead(PIN_EXIT_GND) == LOW) {
      delay(50);
      if (digitalRead(PIN_EXIT_GND) == LOW) {
        running = false;
        break;
      }
    }

    if (digitalRead(PIN_SW) == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(PIN_SW) == LOW) {
        if (millis() - pressStart >= 150) {
          app.onHold();
          app.update();
          delay(40);
        }
      }
      unsigned long duration = millis() - pressStart;

      if (duration < 150) {
        app.onClick();
      }
    }

    app.update();
  }
}

void executeCommand(String cmd, String arg = "") {
  cmd.trim();
  arg.trim();

  if (cmd == "CLEAR") {
    lastOutput = "";
    return;
  }

  if (cmd == "DXDIAG") {
    lastOutput = "Running DXDIAG...";
    runDxdiag();
    lastOutput = "Returned from DXDIAG";
    return;
  }

  if (cmd == "SERIAL") {
    lastOutput = "Entering SERIAL Mode...";
    runSerial();
    lastOutput = "Exited SERIAL Mode";
    return;
  }

  if (cmd == "START") {
    for (int i = 0; i < argCount; i++) {
      if (arg == appTable[i].name) {
        lastOutput = "Running " + arg + "...";
        appTable[i].runner();
        lastOutput = "Returned from " + arg;
        return;
      }
    }
    lastOutput = "File not found";
    return;
  }

  lastOutput = "Bad command or file name";
}

void setup() {
  u8g2.begin();
  io.begin();
  Serial.begin(115200);

  pinMode(PIN_EXIT_GND, INPUT_PULLUP);

  // EEPROMからWi-Fi情報を読み込んで接続
  WifiCredentials creds = WifiConfig::load();
  if (creds.valid) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 20, "Connecting Wi-Fi...");
    u8g2.drawStr(0, 35, creds.ssid);
    u8g2.sendBuffer();

    WiFi.begin(creds.ssid, creds.pass);
    
    // 5秒間だけ接続試行（タイムアウト付き）
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 10) {
      delay(500);
      timeout++;
    }
  }
}

void loop() {
  // io.updateLED();

  int dir = io.readEncoder();
  if (dir != 0) {
    if (selectingCommand) {
      currentWordIdx = (currentWordIdx + dir + cmdCount) % cmdCount;
    } else {
      currentWordIdx = (currentWordIdx + dir + argCount) % argCount;
    }
  }

  if (digitalRead(PIN_SW) == LOW) {
    unsigned long pressStart = millis();
    while (digitalRead(PIN_SW) == LOW);
    unsigned long duration = millis() - pressStart;

    if (duration >= 800) {
      if (!selectingCommand) {
        executeCommand("START", appTable[currentWordIdx].name);
        selectingCommand = true;
        currentWordIdx = 0;
      }
    } else {
      if (selectingCommand) {
        String selectedCmd = cmdList[currentWordIdx];
        if (selectedCmd == "START") {
          selectingCommand = false;
          currentWordIdx = 0;
        } else {
          executeCommand(selectedCmd);
          currentWordIdx = 0;
        }
      } else {
        executeCommand("START", appTable[currentWordIdx].name);
        selectingCommand = true;
        currentWordIdx = 0;
      }
    }
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);

  u8g2.drawStr(0, 8, "Arduino OS v1.0.0");
  u8g2.drawHLine(0, 10, 128);
  io.drawWifiIcon(118, 1);

  if (lastOutput.length() > 0) {
    u8g2.setCursor(0, 22);
    u8g2.print(lastOutput);
  }

  u8g2.setCursor(0, 38);
  u8g2.print(currentPrompt);
  
  if (selectingCommand) {
    u8g2.print(" [");
    u8g2.print(cmdList[currentWordIdx]);
    u8g2.print("]");
  } else {
    u8g2.print(" START [");
    u8g2.print(appTable[currentWordIdx].name);
    u8g2.print("]");
  }

  u8g2.drawHLine(0, 56, 128);  

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 64, "TURN:SELECT / CLICK:ENTER");
  u8g2.sendBuffer();
}
