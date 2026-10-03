#ifndef TETRIS_H
#define TETRIS_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "../lib/io.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern IO io;

class AppTetris {
private:
  static const int BOARD_WIDTH = 10;
  static const int BOARD_HEIGHT = 20;
  static const int BLOCK_SIZE = 5;

  uint8_t board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

  const uint8_t SHAPES[7][4][4] = {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, // I
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, // O
    {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}}, // T
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}}, // S
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}, // Z
    {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}}, // J
    {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}  // L
  };

  int currentPiece = 0;
  int nextPiece = 0;
  int pieceX = 3;
  int pieceY = 0;
  uint8_t currentShape[4][4];

  unsigned long lastDropTime = 0;
  int dropInterval = 400;

  bool isGrounded = false;
  unsigned long lockStartTime = 0;
  const unsigned long LOCK_DELAY = 500;

  int score = 0;
  bool gameOver = false;

  void spawnPiece() {
    currentPiece = nextPiece;
    nextPiece = random(0, 7);
    pieceX = 3;
    pieceY = 0;
    isGrounded = false;

    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        currentShape[r][c] = SHAPES[currentPiece][r][c];
      }
    }

    if (checkCollision(pieceX, pieceY, currentShape)) {
      gameOver = true;
    }
  }

  bool checkCollision(int nx, int ny, uint8_t shape[4][4]) {
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        if (shape[r][c]) {
          int bx = nx + c;
          int by = ny + r;

          if (bx < 0 || bx >= BOARD_WIDTH || by >= BOARD_HEIGHT) {
            return true;
          }
          if (by >= 0 && board[by][bx]) {
            return true;
          }
        }
      }
    }
    return false;
  }

  // SRS（スーパーローテーションシステム）対応の回転処理
  void rotatePiece() {
    uint8_t temp[4][4];
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        temp[c][3 - r] = currentShape[r][c];
      }
    }

    // キック候補オフセット (dx, dy)
    // 通常(0,0)、左右シフト、上方向キックなどを試行
    const int kickOffsets[5][2] = {
      {0, 0},
      {-1, 0},
      {1, 0},
      {0, -1},
      {-2, 0}
    };

    for (int i = 0; i < 5; i++) {
      int testX = pieceX + kickOffsets[i][0];
      int testY = pieceY + kickOffsets[i][1];

      if (!checkCollision(testX, testY, temp)) {
        pieceX = testX;
        pieceY = testY;

        for (int r = 0; r < 4; r++) {
          for (int c = 0; c < 4; c++) {
            currentShape[r][c] = temp[r][c];
          }
        }

        // 接地中に回転成功したら猶予時間を再リセット
        if (checkCollision(pieceX, pieceY + 1, currentShape)) {
          isGrounded = true;
          lockStartTime = millis();
        }
        break;
      }
    }
  }

  void lockPiece() {
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        if (currentShape[r][c]) {
          int by = pieceY + r;
          int bx = pieceX + c;
          if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH) {
            board[by][bx] = 1;
          }
        }
      }
    }
    clearLines();
    spawnPiece();
  }

  void clearLines() {
    for (int r = BOARD_HEIGHT - 1; r >= 0; r--) {
      bool full = true;
      for (int c = 0; c < BOARD_WIDTH; c++) {
        if (!board[r][c]) {
          full = false;
          break;
        }
      }

      if (full) {
        score += 100;
        for (int nr = r; nr > 0; nr--) {
          for (int c = 0; c < BOARD_WIDTH; c++) {
            board[nr][c] = board[nr - 1][c];
          }
        }
        for (int c = 0; c < BOARD_WIDTH; c++) {
          board[0][c] = 0;
        }
        r++;
      }
    }
  }

  int getGhostY() {
    int gy = pieceY;
    while (!checkCollision(pieceX, gy + 1, currentShape)) {
      gy++;
    }
    return gy;
  }

public:
  void init() {
    memset(board, 0, sizeof(board));
    score = 0;
    gameOver = false;
    lastDropTime = millis();
    nextPiece = random(0, 7);
    spawnPiece();
  }

  void onClick() {
    if (gameOver) {
      init();
    } else {
      rotatePiece();
    }
  }

  void onHold() {
    if (!gameOver) {
      if (!checkCollision(pieceX, pieceY + 1, currentShape)) {
        pieceY++;
        score += 1;
        lastDropTime = millis();
      } else {
        // 接地中の押し下げる操作時にも猶予を延長
        lockStartTime = millis();
      }
    }
  }

  void update() {
    if (!gameOver) {
      int dir = io.readEncoder();
      if (dir != 0) {
        if (!checkCollision(pieceX - dir, pieceY, currentShape)) {
          pieceX -= dir;
          // 接地中に移動成功したら猶予時間を再リセット（延長）
          if (checkCollision(pieceX, pieceY + 1, currentShape)) {
            isGrounded = true;
            lockStartTime = millis();
          }
        }
      }

      bool touchGround = checkCollision(pieceX, pieceY + 1, currentShape);

      if (touchGround) {
        if (!isGrounded) {
          isGrounded = true;
          lockStartTime = millis();
        } else if (millis() - lockStartTime >= LOCK_DELAY) {
          lockPiece();
        }
      } else {
        isGrounded = false;
      }

      if (!isGrounded && (millis() - lastDropTime > dropInterval)) {
        lastDropTime = millis();
        pieceY++;
      }
    }

    u8g2.setDisplayRotation(U8G2_R3);
    u8g2.clearBuffer();

    // 盤面描画（左上に配置: 10列×20行×5px = 50px×100px）
    int offsetX = 1;
    int offsetY = 2;

    u8g2.drawFrame(offsetX - 1, offsetY - 1, BOARD_WIDTH * BLOCK_SIZE + 2, BOARD_HEIGHT * BLOCK_SIZE + 2);

    for (int r = 0; r < BOARD_HEIGHT; r++) {
      for (int c = 0; c < BOARD_WIDTH; c++) {
        if (board[r][c]) {
          u8g2.drawBox(offsetX + c * BLOCK_SIZE, offsetY + r * BLOCK_SIZE, BLOCK_SIZE - 1, BLOCK_SIZE - 1);
        }
      }
    }

    if (!gameOver) {
      // ゴースト表示
      int ghostY = getGhostY();
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          if (currentShape[r][c]) {
            int gx = offsetX + (pieceX + c) * BLOCK_SIZE;
            int gy = offsetY + (ghostY + r) * BLOCK_SIZE;
            u8g2.drawFrame(gx, gy, BLOCK_SIZE, BLOCK_SIZE);
          }
        }
      }

      // 操作中の落下ブロック表示
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          if (currentShape[r][c]) {
            int bx = offsetX + (pieceX + c) * BLOCK_SIZE;
            int by = offsetY + (pieceY + r) * BLOCK_SIZE;
            u8g2.drawBox(bx, by, BLOCK_SIZE - 1, BLOCK_SIZE - 1);
          }
        }
      }
    }

    // =================================================================
    // 画面下の空きエリア（Y: 104px〜127px）を使ったスコア＆NEXT表示
    // =================================================================
    u8g2.setFont(u8g2_font_5x7_tr);

    // SCORE表示 (左下)
    u8g2.setCursor(2, 112);
    u8g2.print("SCORE");
    u8g2.setCursor(2, 123);
    u8g2.print(score);

    // NEXT表示ラベル (右下)
    u8g2.setCursor(38, 112);
    u8g2.print("NEXT");

    // NEXTミノ描画（右下: 4x4ドットブロックでハッキリ表示）
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        if (SHAPES[nextPiece][r][c]) {
          u8g2.drawBox(42 + c * 4, 114 + r * 4, 3, 3);
        }
      }
    }

    if (gameOver) {
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(12, 50, "GAME OVER");
    }

    u8g2.sendBuffer();

    u8g2.setDisplayRotation(U8G2_R0);
  }
};

#endif
