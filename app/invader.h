#ifndef INVADER_H
#define INVADER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppInvader {
private:
  static const int COLS = 5;
  static const int ROWS = 3;
  
  // インベーダーの存在フラグ (1: 生存, 0: 撃破)
  uint8_t enemies[ROWS][COLS];
  int enemyX = 0;
  int enemyY = 10;
  int enemyDir = 1; // 1: 右, -1: 左
  unsigned long lastEnemyMove = 0;
  int enemySpeed = 400; // 移動間隔(ms)

  // 自機
  int playerX = 28;
  const int playerY = 118;

  // 自機の弾
  int bulletX = -1;
  int bulletY = -1;

  // 敵の弾
  int eBulletX = -1;
  int eBulletY = -1;

  int score = 0;
  bool gameOver = false;

  void spawnEnemies() {
    for (int r = 0; r < ROWS; r++) {
      for (int c = 0; c < COLS; c++) {
        enemies[r][c] = 1;
      }
    }
    enemyX = 2;
    enemyY = 10;
    enemyDir = 1;
  }

public:
  void init() {
    score = 0;
    gameOver = false;
    bulletY = -1;
    eBulletY = -1;
    playerX = 28;
    spawnEnemies();
  }

  void onClick() {
    if (gameOver) {
      init();
      return;
    }
    // 弾が画面に出ていなければ発射
    if (bulletY < 0) {
      bulletX = playerX + 3;
      bulletY = playerY - 2;
    }
  }

  void onHold() {}

  void update() {
    if (!gameOver) {
      // 1. 自機の移動（エンコーダー）
      int dir = io.readEncoder();
      if (dir != 0) {
        playerX -= dir * 3;
        if (playerX < 0) playerX = 0;
        if (playerX > 56) playerX = 56;
      }

      // 2. 自機弾の移動 & 命中判定
      if (bulletY >= 0) {
        bulletY -= 4;
        if (bulletY < 0) {
          bulletY = -1;
        } else {
          // 敵との当たり判定
          for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
              if (enemies[r][c]) {
                int ex = enemyX + c * 10;
                int ey = enemyY + r * 8;
                if (bulletX >= ex && bulletX <= ex + 6 && bulletY >= ey && bulletY <= ey + 6) {
                  enemies[r][c] = 0;
                  bulletY = -1;
                  score += 10;
                  break;
                }
              }
            }
          }
        }
      }

      // 3. 敵全体の移動
      if (millis() - lastEnemyMove > enemySpeed) {
        lastEnemyMove = millis();
        bool touchWall = false;

        for (int r = 0; r < ROWS; r++) {
          for (int c = 0; c < COLS; c++) {
            if (enemies[r][c]) {
              int ex = enemyX + c * 10 + enemyDir * 2;
              if (ex < 0 || ex > 56) touchWall = true;
            }
          }
        }

        if (touchWall) {
          enemyDir = -enemyDir;
          enemyY += 4; // 端に着いたら一段下がる
        } else {
          enemyX += enemyDir * 2;
        }

        // 敵が自機ラインまで降りてきたらゲームオーバー
        if (enemyY + ROWS * 8 >= playerY) {
          gameOver = true;
        }

        // 敵のランダム攻撃
        if (eBulletY < 0 && random(0, 10) < 3) {
          int randCol = random(0, COLS);
          for (int r = ROWS - 1; r >= 0; r--) {
            if (enemies[r][randCol]) {
              eBulletX = enemyX + randCol * 10 + 3;
              eBulletY = enemyY + r * 8 + 6;
              break;
            }
          }
        }
      }

      // 4. 敵弾の移動 & 被弾判定
      if (eBulletY >= 0) {
        eBulletY += 2;
        if (eBulletY > 128) {
          eBulletY = -1;
        } else if (eBulletX >= playerX && eBulletX <= playerX + 7 && eBulletY >= playerY) {
          gameOver = true; // 被弾
        }
      }

      // 全滅させたかチェック（ウェーブクリアでリセット）
      bool cleared = true;
      for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
          if (enemies[r][c]) cleared = false;
        }
      }
      if (cleared) {
        enemySpeed = max(100, enemySpeed - 50); // スピードアップ
        spawnEnemies();
      }
    }

    // 描画処理
    u8g2.setDisplayRotation(U8G2_R3);
    u8g2.clearBuffer();

    // 敵描画 (6x5ドットの簡易グラフィック)
    for (int r = 0; r < ROWS; r++) {
      for (int c = 0; c < COLS; c++) {
        if (enemies[r][c]) {
          int ex = enemyX + c * 10;
          int ey = enemyY + r * 8;
          u8g2.drawBox(ex + 1, ey, 4, 5);
          u8g2.drawPixel(ex, ey + 2);
          u8g2.drawPixel(ex + 5, ey + 2);
        }
      }
    }

    // 自機描画 (▲形状)
    u8g2.drawTriangle(playerX + 3, playerY, playerX, playerY + 5, playerX + 6, playerY + 5);

    // 弾描画
    if (bulletY >= 0) u8g2.drawBox(bulletX, bulletY, 1, 3);
    if (eBulletY >= 0) u8g2.drawBox(eBulletX, eBulletY, 1, 3);

    // スコア表示
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(0, 6);
    u8g2.print("SCORE:");
    u8g2.print(score);

    if (gameOver) {
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(8, 60, "GAME OVER");
    }

    u8g2.sendBuffer();
    u8g2.setDisplayRotation(U8G2_R0);
  }
};

#endif
