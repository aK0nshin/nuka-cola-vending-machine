void initSpaceInvaders(boolean start) {
  tv.fill(0);

  while (!start) {
    start = titleScreen();
    if (!start) {
      start = displayHighScores(0);
    }
  }
  tv.fill(0);
  tv.select_font(font4x6);

  level = 1;
  remainingLives = 2;
  score = 0;
  initVars();
  drawBunkers();
  oldCannonX = cannonX - 1;
  drawCannon(cannonX, CANNON_Y, 1);
  drawScoreLine();
  drawRemainingLives();
  tv.delay(500);
}

void initVars() {
  clock = 0;
  eraseExplosionTime = -1;
  eraseMysteryScoreTime = -1;
  soundTime = -1;
  bombDropInterval = 20000;
  bombDropTime = bombDropInterval;
  bombDrawTime = BOMB_DRAW_INTERVAL;
  bombY[0] = -1;
  bombY[1] = -1;
  bombY[2] = -1;
  mysteryShipX = W;
  mysteryShipTime = MYSTERY_SHIP_INTERVAL;
  invaderAdvanceInterval = INIT_INVADER_ADVANCE_INTERVAL - ((level - 1) * 200);
  invaderAdvanceTime = invaderAdvanceInterval;
  leftCol = 0;
  rightCol = INVADER_COLS - 1;
  bottomRow = INVADER_ROWS - 1;
  invaderY = 4 + (level - 1) * 2;
  invaderXLeft = 0;
  invaderXRight = (INVADER_COLS * INVADER_WIDTH) - 1;
  for (byte r = 0; r < INVADER_ROWS; r++) {
    invaders[r] = 0xff80;
  }
}

void spaceInvaders() {
  clock++;
  runSchedule();

  if (remainingLives < 0) {
    gameOver();
    initSpaceInvaders(displayHighScores(0));
  }
  if (leftCol > rightCol) {
    newLevel();
  }

  getInput();
  if (scored) {
    scored = false;
    drawScoreLine();
  }
}

boolean pollFireButton(int n) {
  for (int i = 0; i < n; i++) {
    if (useNunchuk) {
      Nunchuk.getData();
    }
    tv.delay(16);
    if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
      return true;
    }
  }
  return false;
}


void newLevel() {
  level++;
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  char s[4];
  sprintf(s, "%d", level);
  tv.print(48, 40, "LEVEL ");
  tv.print(80, 40, s);
  tv.delay(4000);
  tv.fill(0);
  tv.select_font(font4x6);

  initVars();
  drawRemainingLives();
  drawBunkers();
  oldCannonX = cannonX - 1;
  drawCannon(cannonX, CANNON_Y, 1);
}



boolean columnEmpty(byte c) {
  for (byte r = 0; r < INVADER_ROWS; r++) {
    if (invaders[r] & (1 << (15 - c))) {
      return false;
    }
  }
  return true;
}

boolean rowEmpty(byte r) {
  for (byte c = 0; c < INVADER_COLS; c++) {
    if (invaders[r] & (1 << (15 - c))) {
      return false;
    }
  }
  return true;
}


void advanceInvaders() {
  invaderTypeToggle = ++invaderTypeToggle % 2;  // toggle between 0 and 1

  if (invaderTypeToggle == 0) {
    tick();
  }

  if (((invaderXLeft == 0) && (invaderDir < 0)) || ((invaderXRight == W - 1) && (invaderDir > 0))) {
    // reverse direction
    invaderDir = -invaderDir;

    for (byte r = 0; r <= bottomRow; r++) {
      byte offset = r * (INVADER_HEIGHT + ROW_SPACING);
      tv.draw_line(0, invaderY + offset, W - 1, invaderY + offset, 0);
      tv.draw_line(0, invaderY + offset + 1, W - 1, invaderY + offset + 1, 0);
    }
    invaderY += 2;
    if ((invaderY + ((INVADER_HEIGHT + ROW_SPACING) * (bottomRow + 1))) >= (BUNKER_Y + BUNKER_HEIGHT + 4)) {
      drawInvaders();
      // if invaders reach bottom of screen, game is over.
      remainingLives = -1;
      destroyCannon();
    }
  } else {
    invaderXLeft += invaderDir;
    invaderXRight += invaderDir;
  }

  drawInvaders();

}

void damage(byte x, byte y) {
  for (byte yy = (y - 2); yy <= (y + 2); yy++) {
    for (byte xx = (x - 2); xx <= (x + 2); xx++) {
      if (random(0, 1 + abs(xx - x) + abs(yy - y)) < 1) {
        tv.set_pixel(xx, yy, 0);
      }
    }
  }
}

void destroyCannon() {
  for (byte i = 0; i < 200; i++) {
    playTone(random(50, 100), 20);
    tv.set_pixel(random(cannonX, cannonX + CANNON_WIDTH), CANNON_Y + random(0, CANNON_HEIGHT - 1), random(0, 2));
  }
  for (byte y = CANNON_Y; y < (CANNON_Y + 2); y++) {
    for (byte x = cannonX; x < cannonX + CANNON_WIDTH; x++) {
      tv.set_pixel(x, y, 0);
    }
  }
  tv.delay(1000);
  drawCannon(cannonX, CANNON_Y, 1);
}

void gameOver() {
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(game[0])));
  tv.print(44, 40, s);
  strcpy_P(s, (char *)pgm_read_word(&(game[1])));
  tv.print(72, 40, s);
  tv.delay(3000);

  if (score == 0) {
    return;
  }

  enterHighScore(0); // Space Invaders high scores are in file 0
}

void runSchedule() {

  if (clock > invaderAdvanceTime) {
    advanceInvaders();
    invaderAdvanceTime = clock + (invaderAdvanceInterval * speedAdjust);
  }
  if (clock > laserTime) {
    laserTime = clock + (LASER_DRAW_INTERVAL * speedAdjust);
    drawLaser();
  }
  if (clock > eraseExplosionTime) {
    eraseExplosionTime = -1;
    drawBitmap(explosionX, explosionY, BITMAP_BLANK);
    drawInvaders();
  }

  if (clock > eraseMysteryScoreTime) {
    eraseMysteryScoreTime = -1;
    drawBitmap(mysteryShipX, 0, BITMAP_BLANK);
    mysteryShipX = W;
  }

  if (clock > bombDropTime) {
    dropBomb();
    bombDropTime = clock + (bombDropInterval * speedAdjust);
  }

  if (clock > bombDrawTime) {
    bombDrawTime = clock + (BOMB_DRAW_INTERVAL * speedAdjust);
    if ((bombY[0] != -1) || ((bombY[1] != -1)) || ((bombY[2] != -1))) {
      drawBombs();
    }
  }

  if (clock > mysteryShipTime) {
    if (invaderY < 6) {
      mysteryShipTime = clock + (MYSTERY_SHIP_INTERVAL * speedAdjust);
    } else {
      if ((mysteryShipX == W) || (eraseMysteryScoreTime != -1)) {
        // if the mystery ship is not already on screen
        mysteryShipDir = (random(0, 2) == 0) ? -1 : 1;
        if (mysteryShipDir == -1) {
          mysteryShipX = W - MYSTERY_SHIP_WIDTH;
        } else {
          mysteryShipX = 0;
        }
      }
      mysteryShipTime = clock + (MYSTERY_SHIP_DRAW_INTERVAL * speedAdjust);
      handleMysteryShip();
    }
  }

  if (clock > soundTime) {
    playTone(currentFreq, 20, 5);
    currentFreq -= 30;
    soundStep++;
    if (soundStep > 8) {
      soundStep = 0;
      soundTime = -1;
    } else {
      soundTime = clock + (20 * speedAdjust);
    }
  }

}

void handleMysteryShip() {
  if (mysteryShipDir == 1) {
    tv.draw_line(mysteryShipX - 1, 0, mysteryShipX - 1, 3, 0);
  }
  drawBitmap(mysteryShipX, 0, BITMAP_MYSTERY_SHIP);
  mysteryShipX += mysteryShipDir;
  if (mysteryShipX > (W - MYSTERY_SHIP_WIDTH)) {
    mysteryShipX = W;
    drawBitmap(0, 0, BITMAP_BLANK);
    drawBitmap((W - MYSTERY_SHIP_WIDTH), 0, BITMAP_BLANK);
    mysteryShipTime = clock + MYSTERY_SHIP_INTERVAL;
  }
}

void enterInitials() {
  char index = 0;

  tv.fill(0);
  strcpy_P(s, (char *)pgm_read_word(&(strings[0])));
  s[10] = '\0'; // hack: truncate the final 'S' off of the string "HIGH SCORES"
  tv.print(16, 0, s);
  sprintf(s, "%d", score);
  tv.print(88, 0, s);

  initials[0] = ' ';
  initials[1] = ' ';
  initials[2] = ' ';

  while (true) {
    tv.print_char(56, 20, initials[0]);
    tv.print_char(64, 20, initials[1]);
    tv.print_char(72, 20, initials[2]);
    for (byte i = 0; i < 3; i++) {
      tv.draw_line(56 + (i * 8), 27, 56 + (i * 8) + 4, 27, 1);
    }
    tv.draw_line(56, 28, 88, 28, 0);
    tv.draw_line(56 + (index * 8), 28, 56 + (index * 8) + 4, 28, 1);
    tv.delay(160);
    if (useNunchuk) {
      Nunchuk.getData();
    }
    if ((Controller.leftPressed()) || (useNunchuk && (Nunchuk.getJoystickX() < 100))) {
      index--;
      if (index < 0) {
        index = 0;
      } else {
        playTone(1046, 20);
      }
    }
    if ((Controller.rightPressed()) || (useNunchuk && (Nunchuk.getJoystickX() > 200))) {
      index++;
      if (index > 2) {
        index = 2;
      } else {
        playTone(1046, 20);
      }
    }
    if ((Controller.upPressed()) || (useNunchuk && (Nunchuk.getJoystickY() > 200))) {
      initials[index]++;
      playTone(523, 20);
      // A-Z 0-9 :-? !-/ ' '
      if (initials[index] == '0') {
        initials[index] = ' ';
      }
      if (initials[index] == '!') {
        initials[index] = 'A';
      }
      if (initials[index] == '[') {
        initials[index] = '0';
      }
      if (initials[index] == '@') {
        initials[index] = '!';
      }
    }
    if ((Controller.downPressed()) || (useNunchuk && (Nunchuk.getJoystickY() < 100))) {
      initials[index]--;
      playTone(523, 20);
      if (initials[index] == ' ') {
        initials[index] = '?';
      }
      if (initials[index] == '/') {
        initials[index] = 'Z';
      }
      if (initials[index] == 31) {
        initials[index] = '/';
      }
      if (initials[index] == '@') {
        initials[index] = ' ';
      }
    }
    if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
      if (index < 2) {
        index++;
        playTone(1046, 20);
      } else {
        playTone(1046, 20);
        return;
      }
    }
  }

}

void enterHighScore(byte file) {
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file * 10 * 5;
  byte hi, lo;
  char tmpInitials[3];
  unsigned int tmpScore = 0;

  // High score processing
  for (byte i = 0; i < 10; i++) {
    hi = EEPROM.read(address + (5 * i));
    lo = EEPROM.read(address + (5 * i) + 1);
    if ((hi == 0xFF) && (lo == 0xFF)) {
      // The values are uninitialized, so treat this entry
      // as a score of 0.
      tmpScore = 0;
    } else {
      tmpScore = (hi << 8) | lo;
    }
    if (score > tmpScore) {
      enterInitials();
      for (byte j = i; j < 10; j++) {
        hi = EEPROM.read(address + (5 * j));
        lo = EEPROM.read(address + (5 * j) + 1);
        if ((hi == 0xFF) && (lo == 0xFF)) {
          tmpScore = 0;
        } else {
          tmpScore = (hi << 8) | lo;
        }
        tmpInitials[0] = (char)EEPROM.read(address + (5 * j) + 2);
        tmpInitials[1] = (char)EEPROM.read(address + (5 * j) + 3);
        tmpInitials[2] = (char)EEPROM.read(address + (5 * j) + 4);

        // tmpScore and tmpInitials now hold what we want to write in the next slot.

        // write score and initials to current slot
        EEPROM.write(address + (5 * j), ((score >> 8) & 0xFF));
        EEPROM.write(address + (5 * j) + 1, (score & 0xFF));
        EEPROM.write(address + (5 * j) + 2, initials[0]);
        EEPROM.write(address + (5 * j) + 3, initials[1]);
        EEPROM.write(address + (5 * j) + 4, initials[2]);

        score = tmpScore;
        initials[0] = tmpInitials[0];
        initials[1] = tmpInitials[1];
        initials[2] = tmpInitials[2];
      }
      score = 0;
      initials[0] = ' ';
      initials[1] = ' ';
      initials[2] = ' ';
      return;
    }
  }
}

boolean displayHighScores(byte file) {
  byte y = 10;
  byte x = 24;
  char s[16];
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file * 10 * 5;
  byte hi, lo;
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[0])));
  tv.print(40, 0, s);
  for (int i = 0; i < 10; i++) {
    sprintf(s, "%2d", i + 1);
    tv.print(x, y + (i * 8), s);

    hi = EEPROM.read(address + (5 * i));
    lo = EEPROM.read(address + (5 * i) + 1);
    if ((hi == 0xFF) && (lo == 0xFF)) {
      score = 0;
    } else {
      score = (hi << 8) | lo;
    }
    initials[0] = (char)EEPROM.read(address + (5 * i) + 2);
    initials[1] = (char)EEPROM.read(address + (5 * i) + 3);
    initials[2] = (char)EEPROM.read(address + (5 * i) + 4);

    if (score > 0) {
      sprintf(s, "%c%c%c %d", initials[0], initials[1], initials[2], score);
      tv.print(x + 24, y + (i * 8), s);
    }
  }

  if (pollFireButton(500)) {
    return true;
  }
  return false;
}
