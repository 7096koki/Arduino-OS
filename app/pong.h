#ifndef PONG_H
#define PONG_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppPong {
private:
  static const int FIELD_TOP = 11;
  static const int FIELD_BOTTOM = 54;
  static const int FIELD_HEIGHT = FIELD_BOTTOM - FIELD_TOP;
  
  static const int PADDLE_HEIGHT = 12;
  static const int PADDLE_WIDTH = 2;
  
  int playerY = 25;
  int playerScore = 0;
  
  int cpuY = 25;
  int cpuScore = 0;
  
  float ballX = 64;
  float ballY = 32;
  float ballVx = 1.5;
  float ballVy = 1.0;
  
  enum GameState {
    READY,
    PLAYING,
    GAME_OVER
  };
  GameState state = READY;

  void resetBall(bool toPlayer) {
    ballX = 64;
    ballY = FIELD_TOP + (FIELD_HEIGHT / 2);
    ballVx = toPlayer ? -1.5f : 1.5f;
    ballVy = (random(0, 2) == 0 ? 1.0f : -1.0f);
  }

  void updateGame() {
    if (state != PLAYING) return;

    int delta = io.getEncDelta();
    playerY += delta * 3;
    if (playerY < FIELD_TOP) playerY = FIELD_TOP;
    if (playerY > FIELD_BOTTOM - PADDLE_HEIGHT) playerY = FIELD_BOTTOM - PADDLE_HEIGHT;

    int cpuTarget = ballY - (PADDLE_HEIGHT / 2);
    if (cpuY < cpuTarget) cpuY += 1;
    else if (cpuY > cpuTarget) cpuY -= 1;
    if (cpuY < FIELD_TOP) cpuY = FIELD_TOP;
    if (cpuY > FIELD_BOTTOM - PADDLE_HEIGHT) cpuY = FIELD_BOTTOM - PADDLE_HEIGHT;

    ballX += ballVx;
    ballY += ballVy;

    if (ballY <= FIELD_TOP) {
      ballY = FIELD_TOP;
      ballVy *= -1;
    } else if (ballY >= FIELD_BOTTOM - 2) {
      ballY = FIELD_BOTTOM - 2;
      ballVy *= -1;
    }

    if (ballX <= 4 + PADDLE_WIDTH && ballX >= 4) {
      if (ballY + 2 >= playerY && ballY <= playerY + PADDLE_HEIGHT) {
        ballX = 4 + PADDLE_WIDTH;
        ballVx = abs(ballVx) * 1.05f;
        float hitPos = (ballY - playerY) - (PADDLE_HEIGHT / 2.0f);
        ballVy = hitPos * 0.2f;
      }
    }

    if (ballX >= 122 - PADDLE_WIDTH && ballX <= 122) {
      if (ballY + 2 >= cpuY && ballY <= cpuY + PADDLE_HEIGHT) {
        ballX = 122 - PADDLE_WIDTH;
        ballVx = -abs(ballVx) * 1.05f;
        float hitPos = (ballY - cpuY) - (PADDLE_HEIGHT / 2.0f);
        ballVy = hitPos * 0.2f;
      }
    }

    if (ballX < 0) {
      cpuScore++;
      if (cpuScore >= 5) {
        state = GAME_OVER;
      } else {
        state = READY;
        resetBall(true);
      }
    } else if (ballX > 128) {
      playerScore++;
      if (playerScore >= 5) {
        state = GAME_OVER;
      } else {
        state = READY;
        resetBall(false);
      }
    }
  }

public:
  void init() {
    playerY = 25;
    cpuY = 25;
    playerScore = 0;
    cpuScore = 0;
    state = READY;
    resetBall(true);
  }

  void onClick() {
    if (state == READY) {
      state = PLAYING;
    } else if (state == GAME_OVER) {
      init();
    }
  }

  void onHold() {}

  void update() {
    updateGame();

    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(0, 8, "== PONG.EXE ==");
    u8g2.setCursor(90, 8);
    u8g2.print(playerScore);
    u8g2.print("-");
    u8g2.print(cpuScore);
    u8g2.drawHLine(0, 10, 128);

    for (int y = FIELD_TOP; y < FIELD_BOTTOM; y += 4) {
      u8g2.drawVLine(64, y, 2);
    }

    u8g2.drawBox(4, playerY, PADDLE_WIDTH, PADDLE_HEIGHT);
    u8g2.drawBox(122, cpuY, PADDLE_WIDTH, PADDLE_HEIGHT);
    u8g2.drawBox((int)ballX, (int)ballY, 2, 2);

    if (state == READY) {
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(22, 35, "CLICK TO SERVE");
    } else if (state == GAME_OVER) {
      u8g2.setFont(u8g2_font_6x10_tr);
      if (playerScore >= 5) {
        u8g2.drawStr(38, 30, "YOU WIN!");
      } else {
        u8g2.drawStr(36, 30, "YOU LOSE");
      }
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(28, 45, "CLICK TO RESTART");
    }

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawHLine(0, 55, 128);
    u8g2.drawStr(0, 63, "ROT:MOVE / CLICK:SERVE");

    u8g2.sendBuffer();
  }
};

#endif
