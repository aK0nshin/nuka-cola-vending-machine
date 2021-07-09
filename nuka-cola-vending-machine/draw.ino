void drawWelcome() {
  tv.fill(0);
  for (byte i = 0; i < 3; i++) {
    strcpy_P(s, (char *)pgm_read_word(&(welcome[i])));
    tv.print(32, 30 + (i * 8), s);
  }
  if (coinsCount != 0) {
    strcpy_P(s, (char *)pgm_read_word(&(welcome[3])));
    tv.print(32, 30 + (3 * 8), s);
    tv.print(coinsCount, 10);
  }
}

void drawFrame() {
  tv.draw_line(0, 0, W - 1, 0, 1);
  tv.draw_line(0, 0, 0, H - 1, 1);
  tv.draw_line(W - 1, H - 1, W - 1, 0, 1);
  tv.draw_line(W - 1, H - 1, 0, H - 1, 1);
}

void drawArrow(byte x, byte y) {
  tv.set_pixel(x + 4, y, 1);
  tv.set_pixel(x + 5, y + 1, 1);
  tv.draw_line(x, y + 2, x + 6, y + 2, 1);
  tv.set_pixel(x + 5, y + 3, 1);
  tv.set_pixel(x + 4, y + 4, 1);
}

void eraseArrow(byte x, byte y) {
  for (byte j = 0; j < 8; j++) {
    tv.draw_line(x, y + j, x + 7, y + j, 0);
  }
}

void drawMenu() {
  tv.fill(0);
  byte text_x = 32;
  byte text_y = 30;

  for (byte i = 0; i < 5; i++) {
    strcpy_P(s, (char *)pgm_read_word(&(menu[i])));
    tv.print(text_x, text_y + (i * 8), s);
  }
  switchArrow(4);
}

void switchArrow(byte place) {
  byte arrow_x = 24;
  byte arrow_y = 30;
  for (byte i = 0; i < 5; i++) {
    arrow_y = arrow_y + (i * 8);
    if (i == place) {
      drawArrow(arrow_x, arrow_y);
    } else {
      eraseArrow(arrow_x, arrow_y);
    }
  }
}


void drawLaser() {
  byte x, y;
  byte r, c;

  if (fired) {
    // erase lowest pixel of laser
    tv.set_pixel(laserX, laserY + 2, 0);
    laserY--;

    if (tv.get_pixel(laserX, laserY) != 0) {
      // we hit something!
      tv.set_pixel(laserX, laserY, 0);
      tv.set_pixel(laserX, laserY + 1, 0);
      tv.set_pixel(laserX, laserY + 2, 0);
      fired = false;
      laserTime = -1;

      for (r = 0; r <= bottomRow; r++) {
        for (c = leftCol; c <= rightCol; c++) {
          if (invaders[r] & (1 << (15 - c))) {
            // (x,y) is upper left corner of invader
            x = invaderXLeft + ((c - leftCol) * (INVADER_WIDTH + COL_SPACING));
            y = invaderY + (r * (INVADER_HEIGHT + ROW_SPACING));
            if ((laserX >= x) && (laserX <= x + INVADER_WIDTH) && (laserY >= y) && (laserY <= y + INVADER_HEIGHT)) {
              // We hit an invader!
              invaders[r] &= ~(1 << 15 - c);
              if (eraseExplosionTime != -1) {
                // There is already an explosion displayed.  Erase it.
                drawBitmap(explosionX, explosionY, BITMAP_BLANK);
              }
              drawBitmap(x, y, BITMAP_EXPLOSION);

              soundTime = clock;
              currentFreq = 1046;

              eraseExplosionTime = clock + 1000;
              explosionX = x;
              explosionY = y;
              bombDropInterval -= 500;
              if (bombDropInterval < 1000) {
                bombDropInterval = 1000;
              }

              invaderAdvanceInterval -= 60;
              if (invaderAdvanceInterval < 30) {
                invaderAdvanceInterval = 30;
              }
              score += 10;
              if (r <= 2) {
                score += 10;
              }
              if (r == 0) {
                score += 10;
              }
              scored = true;

              // Adjust values of leftCol, rightCol, and bottomRow if needed
              // If we destroyed the last invader, then leftCol > rightCol.
              if (c == leftCol) {
                while ((columnEmpty(leftCol)) && (leftCol < INVADER_COLS)) {
                  invaderXLeft += (INVADER_WIDTH + COL_SPACING);
                  leftCol++;
                }
              }
              if (c == rightCol) {
                while ((columnEmpty(rightCol)) && (rightCol > 0)) {
                  invaderXRight -= (INVADER_WIDTH + COL_SPACING);
                  rightCol--;
                }
              }
              if (r == bottomRow) {
                while ((rowEmpty(bottomRow)) && (bottomRow > 0)) {
                  bottomRow--;
                }
              }
              return;
            }
          }
        }
      }

      // Check to see if we hit a falling bomb.
      for (byte i = 0; i < 3; i++) {
        if ((laserX == bombX[i]) && (laserY <= bombY[i]) && (laserY >= bombY[i] - 2)) {
          // erase the laser
          tv.set_pixel(laserX, laserY, 0);
          tv.set_pixel(laserX, laserY + 1, 0);
          tv.set_pixel(laserX, laserY + 2, 0);
          // erase the bomb
          tv.set_pixel(bombX[i], bombY[i], 0);
          tv.set_pixel(bombX[i], bombY[i] - 1, 0);
          tv.set_pixel(bombX[i], bombY[i] - 2, 0);
          bombY[i] = -1;
          return;
        }
      }

      // Check to see if we hit the mystery ship
      if ((laserY < 5) && (laserY >= 0) && (laserX >= mysteryShipX) && (laserX < mysteryShipX + MYSTERY_SHIP_WIDTH) && (eraseMysteryScoreTime == -1)) {
        playTone(1046, 20);
        tv.delay(16);
        playTone(1318, 20);
        tv.delay(16);
        playTone(1568, 20);
        tv.delay(16);
        playTone(2093, 20);

        byte bitmap = BITMAP_BLANK + 8;
        byte mysteryScore = random(0, 4);
        bitmap += mysteryScore * 8;
        switch (mysteryScore) {
          case 0:
            score += 50;
            break;
          case 1:
            score += 100;
            break;
          case 2:
            score += 150;
            break;
          case 3:
            score += 300;
        }
        scored = true;
        drawBitmap(mysteryShipX, 0, bitmap);
        tv.draw_line(mysteryShipX - 1, 0, mysteryShipX - 1, 3, 0);
        eraseMysteryScoreTime = clock + 10000;
        mysteryShipTime = clock + MYSTERY_SHIP_INTERVAL;
        return;
      }

      // We hit a bunker.
      if (laserY >= BUNKER_Y) {
        damage(laserX, laserY);
      }
      return;
    }

    if (laserY >= 0) {
      tv.set_pixel(laserX, laserY, 1);
    } else {
      // laser has reached top of screen
      tv.set_pixel(laserX, laserY + 1, 0);
      tv.set_pixel(laserX, laserY + 2, 0);
      fired = false;
      laserTime = -1;
    }
  }
}

void drawBunkers() {
  for (byte i = 0; i < 4; i++) {
    drawBunker(((W / 4)*i) +  (W / 4 / 2) - (BUNKER_WIDTH / 2), BUNKER_Y);
  }
}

void drawBunker(byte x, byte y) {
  tv.draw_line(x + 3, y, x + BUNKER_WIDTH - 3, y, 1);
  tv.draw_line(x + 2, y + 1, x + BUNKER_WIDTH - 2, y + 1, 1);
  tv.draw_line(x + 1, y + 2, x + BUNKER_WIDTH - 1, y + 2, 1);
  for (byte r = y + 3; r < (y + BUNKER_HEIGHT); r++) {
    tv.draw_line(x, r, x + BUNKER_WIDTH, r, 1);
  }
  //  tv.draw_line(x+7, y+BUNKER_HEIGHT-4, x+9, y+BUNKER_HEIGHT-4, 0);
  tv.draw_line(x + 5, y + BUNKER_HEIGHT - 3, x + 9, y + BUNKER_HEIGHT - 3, 0);
  tv.draw_line(x + 4, y + BUNKER_HEIGHT - 2, x + 10, y + BUNKER_HEIGHT - 2, 0);
  tv.draw_line(x + 4, y + BUNKER_HEIGHT - 1, x + 10, y + BUNKER_HEIGHT - 1, 0);
}

void dropBomb() {
  boolean done = false;
  char c, r;
  byte i = 0;
  while (bombY[i] != -1) {
    i++;
    if (i >= 3) {
      return;
    }
  }

  while (!done) {
    // randomly choose a column of invaders
    c = (byte)random(leftCol, rightCol + 1);
    // look for the bottom-most invader in that column
    // note that there may not be any invaders in the column
    for (r = bottomRow; r >= 0; r--) {
      if (invaders[r] & (1 << (15 - c))) {
        done = true;
        break; // break out of for loop
      }
    }
  }
  bombX[i] = invaderXLeft + ((c - leftCol) * (INVADER_WIDTH + COL_SPACING)) + INVADER_WIDTH / 2;
  bombY[i] = invaderY + ((r + 1) * (INVADER_HEIGHT + ROW_SPACING)) - 2;
  tv.set_pixel(bombX[i], bombY[i], 1);
  tv.set_pixel(bombX[i], bombY[i] - 1, 1);
  tv.set_pixel(bombX[i], bombY[i] - 2, 1);
}

void drawBombs() {
  // draw the bombs and detect collisions
  for (byte i = 0; i < 3; i++) {
    if (bombY[i] == -1) {
      continue;
    }
    tv.set_pixel(bombX[i], bombY[i] - 2, 0);
    bombY[i]++;

    if (bombY[i] >= FOOTER_Y) {
      // bomb has reached bottom of screen
      tv.set_pixel(bombX[i], bombY[i] - 1, 0);
      tv.set_pixel(bombX[i], bombY[i] - 2, 0);
      bombY[i] = -1;
      return;
    }

    if (tv.get_pixel(bombX[i], bombY[i]) == 1) {
      // bomb hit something!
      if ((bombX[i] == laserX) && (bombY[i] == laserY)) {
        // collision between laser and bomb is handled in drawLaser()
        return;
      }

      // process collisions
      if ((bombX[i] >= cannonX) && (bombX[i] < cannonX + CANNON_WIDTH) && (bombY[i] >= CANNON_Y) && (bombY[i] <= CANNON_Y + CANNON_HEIGHT)) {
        // laser cannon destroyed!
        tv.set_pixel(bombX[i], bombY[i], 0);
        tv.set_pixel(bombX[i], bombY[i] - 1, 0);
        tv.set_pixel(bombX[i], bombY[i] - 2, 0);
        destroyCannon();
        fired = false;
        tv.set_pixel(laserX, laserY, 0);
        tv.set_pixel(laserX, laserY + 1, 0);
        tv.set_pixel(laserX, laserY + 2, 0);
        remainingLives--;
        drawRemainingLives();
        bombY[i] = -1;
        return;
      }

      tv.set_pixel(bombX[i], bombY[i], 0);
      tv.set_pixel(bombX[i], bombY[i] - 1, 0);
      tv.set_pixel(bombX[i], bombY[i] - 2, 0);
      damage(bombX[i], bombY[i]);

      bombY[i] = -1;
      return;
    }

    tv.set_pixel(bombX[i], bombY[i], 1);
  }

}

void drawScore() {
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[10])));
  tv.print(5, 30, s);
  tv.delay(1000);

  //tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[11])));
  tv.print(5, 40, s);
  tv.delay(1000);

  // tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[12])));
  tv.print(5, 50, s);
  tv.delay(10000);
  tv.fill(0);
}

void drawScoreLine() {
  // First, erase;
  for (byte y = FOOTER_Y; y < H; y++) {
    tv.draw_line(W / 2, FOOTER_Y + y, W - 1, FOOTER_Y + y, 0);
  }
  sprintf(s, "%d", score);
  byte len = strlen(s);
  byte xx = (W  - (len * 4) - 1);
  tv.print(xx, FOOTER_Y, s);
}

void drawCannon(byte x, byte y, byte color) {
  tv.set_pixel(x + CANNON_MUZZLE, y, color);
  for (byte r = y + 1; r < (y + CANNON_HEIGHT); r++) {
    for (byte c = x; c < x + CANNON_WIDTH; c++) {
      tv.set_pixel(c, r, color);
    }
  }
}

void drawRemainingLives() {
  for (byte i = 0; i < 3; i++) {
    drawCannon(i * (CANNON_WIDTH + 1), H - CANNON_HEIGHT - 1, 0);
  }
  for (byte i = 0; i < remainingLives; i++) {
    drawCannon(i * (CANNON_WIDTH + 1), H - CANNON_HEIGHT - 1, 1);
  }
}

void drawInvaders() {
  byte x;
  byte y;

  for (byte r = 0; r <= bottomRow; r++) {
    for (byte c = leftCol; c <= rightCol; c++) {

      //smooths input
      for (byte i = 0; i < 5; i++) {
        getInput();
      }

      x = invaderXLeft + ((c - leftCol) * (INVADER_WIDTH + COL_SPACING));
      y = invaderY + (r * (INVADER_HEIGHT + ROW_SPACING));
      if (invaders[r] & (1 << (15 - c))) {
        clock += 10;
        // invader here, so draw it
        tv.draw_line(x - invaderDir, y, x - invaderDir, y + INVADER_HEIGHT - 1, 0);
        switch (r) {
          case 0:
            drawBitmap(x, y, (invaderTypeToggle * 8)); // invader type 0
            break;
          case 1:
          case 2:
            drawBitmap(x, y, 16 + (invaderTypeToggle * 8)); // invader type 1
            break;
          case 3:
          case 4:
            drawBitmap(x, y, 32 + (invaderTypeToggle * 8)); // invader type 2
            break;
        }
      }
    }
  }
}


void drawBitmap(byte x, byte y, unsigned int bitmapIndex) {
  //const prog_uint16_t *index = bitmaps + bitmapIndex;
  const uint16_t *index = bitmaps + bitmapIndex;
  unsigned int ii;
  byte hi, lo;
  byte pixelOffset;
  int byteIndex;
  byte rows = 8;

  if ((x >= W) || (y >= H)) {
    return;
  }

  if (bitmapIndex == BITMAP_MYSTERY_SHIP) {
    rows = 5;
  }

  byteIndex = (y * (W / 8)) + (x / 8);
  pixelOffset = x % 8;
  boolean nowrap1 = ((byteIndex + 1) % (W / 8)) != 0;
  boolean nowrap2 = ((byteIndex + 2) % (W / 8)) != 0;
  for (byte l = 0; l < rows; l++) {
    ii = pgm_read_word(index + l);
    hi = ((ii >> 8) & 0xFF);
    lo = (ii & 0xFF);
    display.screen[byteIndex] &= (0xFF << (8 - pixelOffset)); // keep upper 'pixelOffset' bits
    display.screen[byteIndex] |= (hi >> pixelOffset);
    if (pixelOffset <= 4) {
      if (nowrap1) {
        display.screen[byteIndex + 1] &= (0x0F >> pixelOffset); // keep lower 4-pixelOffset bits
        display.screen[byteIndex + 1] = ((hi << (8 - pixelOffset) | (lo >> pixelOffset)));
      }
    } else {
      // The 12 bit row spans 3 bytes
      if (nowrap1) {
        display.screen[byteIndex + 1] = (hi << (8 - pixelOffset)) | (lo >> pixelOffset);
        if (nowrap2) {
          display.screen[byteIndex + 2] &= (0xFF >> (pixelOffset - 4));
          display.screen[byteIndex + 2] |= (lo << (8 - pixelOffset));
        }
      }
    }
    byteIndex += (W / 8);
  }
}
