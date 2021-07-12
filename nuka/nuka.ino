/*
  Hackvision Firmware
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/
#include <avr/pgmspace.h>
#include <TVout.h>
#include <video_gen.h>
#include <font4x6.h>
#include <font6x8.h>
#include <EEPROM.h>
#include "src/Controllers/Controllers.h"
#define W 136
#define H 98
#define LEFT_BUTTON 4
#define RIGHT_BUTTON 5
#define UP_BUTTON 10
#define DOWN_BUTTON 3
#define FIRE_BUTTON 6
#define JUMP_BUTTON 8
#define BOTTLE_PIN 13
#define COIN_PIN 2
//#define REMOTE_PIN 17

#define CANNON_WIDTH 7
#define CANNON_MUZZLE 3
#define CANNON_HEIGHT 3
#define CANNON_Y 88
#define LASER_HEIGHT 3
#define FOOTER_Y 92
#define INVADER_ROWS 5
#define INVADER_COLS 9
#define ROW_SPACING 2
#define COL_SPACING 0
#define BUNKER_WIDTH 14
#define BUNKER_HEIGHT 10
#define BUNKER_Y 76
#define INVADER_HEIGHT 8
#define INVADER_WIDTH 12
//#define DEBUG_INTERVAL 50000
#define LASER_DRAW_INTERVAL 75
#define BOMB_DRAW_INTERVAL 300
#define MYSTERY_SHIP_WIDTH 11
#define MYSTERY_SHIP_DRAW_INTERVAL 300
#define MYSTERY_SHIP_INTERVAL 75000
#define BITMAP_MYSTERY_SHIP 48
#define BITMAP_EXPLOSION 56
#define BITMAP_BLANK 64
#define BITMAP_INVERTED_Y 104
#define BITMAP_CENT 120
#define INIT_INVADER_ADVANCE_INTERVAL 3000
#define POINTS_TO_BOTTLE 6500

const uint16_t bitmaps[] PROGMEM = {
#include "bitmaps.h"
};

typedef void (*func_type)(void);
void shitBottle();
void goToGame();
void exitToTitle();
void backToGame();

const char w0[] PROGMEM = "INSERT COINS";
const char w1[] PROGMEM = "Coins in: ";
const char* const menuTitleStrings[] PROGMEM = {w0, w1};

const char m0[] PROGMEM = "NUKA-COLA ";
const char m1[] PROGMEM = "SPACE INVADERS ";
const char* const menuStrings[] PROGMEM = {m0, m1};
const uint8_t price[] = {2, 1};
const uint8_t menuPositionsCount = 2;
const func_type menuHandlers[] = {shitBottle, goToGame};

const char t0[] PROGMEM = "NUKA-COLA";
const char t1[] PROGMEM = "vs";
const char* const titleStrings[] PROGMEM = {t0, t1};

const char pt0[] PROGMEM = "PAUSE";
const char* const pauseTitleStrings[] PROGMEM = {pt0};

const char p0[] PROGMEM = "Back";
const char p1[] PROGMEM = "Exit";
const char* const pauseStrings[] PROGMEM = {p0, p1};
const func_type pauseHandlers[] = {backToGame, exitToTitle};

const char s0[] PROGMEM = "HIGH SCORES";
const char s1[] PROGMEM = "PLA";
const char s2[] PROGMEM = "SPACE INVADERS";
const char s3[] PROGMEM = " =  10";
const char s4[] PROGMEM = " =  20";
const char s5[] PROGMEM = " =  30";
const char s6[] PROGMEM = " =  ??";
const char s7[] PROGMEM = "GAME";
const char s8[] PROGMEM = "OVER";
const char s9[] PROGMEM = "VAULT-TEC";
const char s10[] PROGMEM = "YOU GOT NUKED";
const char s11[] PROGMEM = "qwertyuiopasdfghjk";
const char s12[] PROGMEM = "18 bukv ili cifr..";
//strcpy_P(s, (char *)pgm_read_word(&(strings[
const char* const strings[] PROGMEM = {s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12};

boolean useNunchuk = false;
char s[16]; // general string buffer
boolean scored = false;
byte tickFreq[4] = {73, 65, 49, 55};
unsigned long clock = 0;
byte cannonX = 60;
int cannonXF = 6000;
byte oldCannonX = 0;
unsigned int invaders[INVADER_ROWS];
byte bottomRow = INVADER_ROWS - 1;
byte leftCol = 0; // leftmost column of invaders
byte rightCol = INVADER_COLS - 1; // rightmost column of invaders
char invaderDir = 1;
byte invaderXLeft = 0;
byte invaderXRight = (INVADER_COLS * INVADER_WIDTH) - 1;
byte invaderY;
byte invaderTypeToggle = 0;
int invaderAdvanceInterval = INIT_INVADER_ADVANCE_INTERVAL;
byte explosionX, explosionY;
byte mysteryShipX = W;
char mysteryShipDir;
byte currentTonePriority = 0;
byte tickFreqCounter = 0;
byte level = 1;

volatile uint8_t coinsCount = 0;
uint8_t lastCoins = 0;
uint8_t myState = 0;
byte choice = 0;
unsigned long welcomeClock = 0;
unsigned long lastCoinTime = 0;
unsigned long lastMenuTime = 0;
unsigned long lastRemoteTime = 0;

bool (*game)();
TVout tv;

byte laserX;
char laserY;
byte bombX[3];
char bombY[3] = { -1, -1, -1};
boolean fired = false;
int currentFreq = 0;
byte soundStep = 0;

unsigned int score;
char remainingLives;
char initials[3];

//unsigned long debugTime = 2000;
unsigned long invaderAdvanceTime = invaderAdvanceInterval;
unsigned long laserTime = -1;
unsigned long mysteryShipTime = MYSTERY_SHIP_INTERVAL;
int bombDropInterval = 20000;
unsigned long bombDropTime = bombDropInterval;
unsigned long bombDrawTime = BOMB_DRAW_INTERVAL;
unsigned long eraseExplosionTime = -1;
unsigned long eraseMysteryScoreTime = -1;
unsigned long soundTime = -1;

// Allow the overall speed of the game to be adjusted.
// Higher number (like 1.5) slow the game down.  Lower numbers (like 0.6) speed it up.
float speedAdjust = 1.0;

void setup()  {
  pinMode(COIN_PIN, INPUT);
  pinMode(BOTTLE_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(COIN_PIN), incCoinCounter, RISING);
  // If pin 12 is pulled LOW, then the PAL jumper is shorted.
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);

  if (digitalRead(12) == LOW) {
    tv.begin(_PAL, W, H);
    // Since PAL processing is faster, we need to slow the game play down.
    speedAdjust = 1.4;
  } else {
    tv.begin(_NTSC, W, H);
  }

  tv.select_font(font6x8);
  randomSeed(analogRead(0));

  playTone(1046, 20);
  tv.delay(16);
  playTone(1318, 20);
  tv.delay(16);
  playTone(1568, 20);
  tv.delay(16);
  playTone(2093, 20);

  // Detect whether nunchuk is connected.  Poll the nunchuk every 4th frame.
  useNunchuk = Nunchuk.init(tv, 4);
  if (useNunchuk) {
    // Speed up game play a bit because of the extra time it takes to
    // communicate with the nunchuk.
    speedAdjust *= 0.8;
  }
  coinsCount = 0;
}

void loop() {
  switch (myState) {
    case 1:
      choice = menu(menuPositionsCount);
      if (coinsCount < price[choice]) {
        for (byte i = 0; i < 10; i++) {
          playTone(random(50, 100), 20);
          tv.delay(1);
        }
        break;
      }
      coinsCount = lastCoins = coinsCount - price[choice];
      menuHandlers[choice]();
      break;
    case 2:
      if (spaceInvaders()) {
        tv.select_font(font6x8);
        myState = 0;
        if (coinsCount > 0) {
          myState = 1;
        }
      }
      break;
//    case 3:
//      closed();
//      break;
    default:
      if (titleScreen()) {
        welcomeClock = millis() + 10000;
        tv.select_font(font6x8);
        myState = 1;
      }
  }
}

//void closed() {
//  tv.fill(0);
//  delay(10000);
//  if (checkRemote) {
//    myState = 0;
//    if (coinsCount > 0) {
//      myState = 1;
//    }
//  }
//}

void exitToTitle() {
  tv.select_font(font6x8);
  myState = 0;
  if (coinsCount > 0) {
    myState = 1;
  }
}

void shitBottle() {
  digitalWrite(BOTTLE_PIN, HIGH);
  delay(500);
  digitalWrite(BOTTLE_PIN, LOW);
  delay(1000);
  myState = 0;
  if (coinsCount > 0) {
    myState = 1;
  }
}

void goToGame() {
  game = &spaceInvaders;
  tv.delay(160);
  initSpaceInvaders(false);
  myState = 2;
}

//boolean checkRemote() {
//  return false;
//  tv.delay(16);
//  if (lastRemoteTime > millis()) {
//    return false;
//  }
//  if (analogRead(REMOTE_PIN) > 800) {
//    if (myState != 3) {
//      myState = 3;
//    }
//    return true;
//  }
//  return false;
//}

boolean checkCoins() {
  if (lastCoins == coinsCount) {
    return false;
  }
  lastCoins = coinsCount;
  playTone(1046, 20);
  tv.delay(16);
  playTone(1318, 20);
  tv.delay(16);
  playTone(1568, 20);
  tv.delay(16);
  playTone(2093, 20);
  if (myState < 1) {
    myState = 1;
  }
  return true;
}

uint8_t jumpPressed(void) {
  if (digitalRead(JUMP_BUTTON) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

boolean pollJumpButton(int n) {
  for (int i = 0; i < n; i++) {
    tv.delay(16);
    if ((jumpPressed())) {
      return true;
    }
  }
  return false;
}

byte menu(byte nChoices) {
  char choice = 0;
  tv.fill(0);
  byte x = 8;
  byte y;

  while (true) {
    strcpy_P(s, (char *)pgm_read_word(&(menuTitleStrings[0])));
    tv.print(32, 10, s);

    for (byte i = 0; i < nChoices; i++) {
      strcpy_P(s, (char *)pgm_read_word(&(menuStrings[i])));
      tv.print(16, 30 + (i * 8), s);
      sprintf(s, "%d", price[i]);
      tv.print(112, 30 + (i * 8), s);
      drawBitmap(120, 30 + (i * 8), BITMAP_CENT);
    }

    strcpy_P(s, (char *)pgm_read_word(&(menuTitleStrings[1])));
    tv.print(32, 78, s);
    sprintf(s, "%d", coinsCount);
    tv.print(s);

    for (byte i = 0; i < nChoices; i++) {
      y = 30 + (i * 8);
      if (i == choice) {
        // draw arrow next to selected game
        tv.set_pixel(x + 4, y, 1);
        tv.set_pixel(x + 5, y + 1, 1);
        tv.draw_line(x, y + 2, x + 6, y + 2, 1);
        tv.set_pixel(x + 5, y + 3, 1);
        tv.set_pixel(x + 4, y + 4, 1);
      } else {
        for (byte j = 0; j < 8; j++) {
          tv.draw_line(x, y + j, x + 7, y + j, 0);
        }
      }
    }
    checkCoins();
//    if (checkRemote) {
//      break;
//    }
    if (coinsCount == 0 && millis() > welcomeClock) {
      myState = 0;
      break;
    }
    // get input
    if (pollFireButton(10)) {
      playTone(1046, 20);
      return choice;
    }
    // note that the call to pollFireButton above got data from the nunchuk device
    if ((Controller.upPressed()) || (useNunchuk && (Nunchuk.getJoystickY() > 200))) {
      choice--;
      if (choice == -1) {
        choice = 0;
      } else {
        playTone(1046, 20);
      }
    }
    if ((Controller.downPressed()) || (useNunchuk && (Nunchuk.getJoystickY() < 100))) {
      choice++;
      if (choice == nChoices) {
        choice = nChoices - 1;
      } else {
        playTone(1046, 20);
      }
    }
  }
}


void runSchedule() {
//  if (checkRemote()) {
//    return;
//  }
  if (clock > invaderAdvanceTime) {
    advanceInvaders();
    invaderAdvanceTime = clock + (invaderAdvanceInterval * speedAdjust);
  }
  if (clock > laserTime) {
    laserTime = clock + (LASER_DRAW_INTERVAL * speedAdjust);
    drawLaser();
  }
  /*
    if (clock > debugTime) {
    //debug(getMemory());
    debugTime = clock + DEBUG_INTERVAL;
    }
  */
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
  //  // This sound is too annoying
  //  if (mysteryShipDir > 0) {
  //  playTone(880 + (constrain((mysteryShipX % 10), 0, 7) * 22), 100);
  //  } else {
  //  playTone(880 + ((9 - constrain((mysteryShipX % 10), 2, 9)) * 22), 100);
  //  }


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

bool spaceInvaders() {
  bool result = false;
  clock++;
  runSchedule();

  if (remainingLives < 0) {
    gameOver();
    initSpaceInvaders(displayHighScores(0));
    result = true;
  }
  if (leftCol > rightCol) {
    newLevel();
  }

  getInput();
  if (scored) {
    scored = false;
    drawScoreLine();
  }
  return result;
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

void gameOver() {
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[7])));
  tv.print(44, 40, s);
  strcpy_P(s, (char *)pgm_read_word(&(strings[8])));
  tv.print(72, 40, s);

  if (score >= POINTS_TO_BOTTLE) {
    strcpy_P(s, (char *)pgm_read_word(&(strings[10])));
    tv.print(28, 60, s);
    shitBottle();
  }

  tv.delay(3000);

  if (score == 0) {
    return;
  }
  enterHighScore(0); // Space Invaders high scores are in file 0
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
//    if (checkRemote()) {
//      return;
//    }
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



boolean titleScreen() {
  byte x, y;
  int d;

  tv.fill(0);
  tv.delay(32);
  tv.select_font(font6x8);

  d = 10;

  strcpy_P(s, (char *)pgm_read_word(&(titleStrings[0])));
  tv.print(42, 0, s);
  strcpy_P(s, (char *)pgm_read_word(&(titleStrings[1])));
  tv.print(62, 10, s);

  if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
    return true;
  }

  for (x = 1; x <= 3; x++) {
    strcpy_P(s, (char *)pgm_read_word(&(strings[1])));
    // cleverness to make the letters appear one at a time.
    // truncate the string with a null character.
    s[x] = '\0';
    tv.print(56, 20, s);
    if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
  }
  drawBitmap(74, 20, BITMAP_INVERTED_Y);
  if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
    return true;
  }


  for (x = 1; x <= 14; x++) {
    if (x == 6) continue; // don't delay on the space char
    strcpy_P(s, (char *)pgm_read_word(&(strings[2])));
    s[x] = '\0';
    tv.print(28, 30, s);
    if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
  }

  x = 74;

  tv.select_font(font4x6);

  d = 60;
  if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
    return true;
  }

  y = 49;
  drawBitmap(48, y, BITMAP_MYSTERY_SHIP);
  strcpy_P(s, (char *)pgm_read_word(&(strings[6])));
  tv.print(64, y, s);
  if (pollFireButton(d) || checkCoins()) {
    return true;
  }
//  checkRemote();


  y = 58;
  drawBitmap(47, y, 0);
  strcpy_P(s, (char *)pgm_read_word(&(strings[5])));
  tv.print(64, y + 1, s);
  if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
    return true;
  }


  y = 68;
  drawBitmap(48, y, 16);
  strcpy_P(s, (char *)pgm_read_word(&(strings[4])));
  tv.print(64, y + 1, s);

  if (pollFireButton(d) || checkCoins()/* || checkRemote()*/) {
    return true;
  }

  y = 78;
  drawBitmap(48, y, 32);
  strcpy_P(s, (char *)pgm_read_word(&(strings[3])));
  tv.print(64, y + 1, s);

  y = 19;
  for (x = W + 6; x > 79; x--) {
    drawBitmap(x, y, BITMAP_BLANK);
    drawBitmap(x, y, 0);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
    x--;
    drawBitmap(x, y, BITMAP_BLANK);
    drawBitmap(x, y, 8);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
  }
  tv.delay(160);

  for (x = 80; x < W + 5; x++) {
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 0);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
    x++;
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 8);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
  }
  tv.delay(320);

  for (x = W + 5; x > 79; x--) {
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y + 8);
    drawBitmap(x, y, 0);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
    x--;

    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y + 8);
    drawBitmap(x, y, 8);
    if (pollFireButton(2) || checkCoins()/* || checkRemote()*/) {
      return true;
    }
  }
  tv.delay(500);
  drawBitmap(80, y, BITMAP_BLANK);

  if (pollFireButton(120) || checkCoins()/* || checkRemote()*/) {
    return true;
  }
  return false;
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

void initSpaceInvaders(boolean start) {
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
  cannonX = 60; // ???
  //  debugTime = DEBUG_INTERVAL;
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

int getMemory() {
  int size = 512;
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  return size;
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

boolean getInput() {
  boolean input = false;

  if (useNunchuk) {
    Nunchuk.getData();
  }

  if (jumpPressed()) {
    exitToTitle();
  }

  if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
    if (!fired) {
      fired = true;
      laserTime = clock;
      laserX = cannonX + (CANNON_WIDTH / 2);
      laserY = CANNON_Y - LASER_HEIGHT;
      input = true;
    }
  }

  if (((Controller.leftPressed()) || (useNunchuk && ((Nunchuk.getJoystickX() < 100) || (Nunchuk.getAccelerometerX() < 100)))) && (cannonX > 0)) {
    oldCannonX = cannonX;
    cannonXF--;
    cannonX = (cannonXF + 5) / 100;
    if (oldCannonX != cannonX) {
      // the cannon moved
      // erase it
      drawCannon(oldCannonX, CANNON_Y, 0);
      oldCannonX = cannonX;
      // draw it
      drawCannon(cannonX, CANNON_Y, 1);
      clock += 60 / speedAdjust;
    }
    return true;
  } else {
    if (((Controller.rightPressed()) || (useNunchuk && ((Nunchuk.getJoystickX() > 200) || (Nunchuk.getAccelerometerX() > 150)))) && (cannonX < (W - CANNON_WIDTH))) {
      oldCannonX = cannonX;
      cannonXF++;
      cannonX = (cannonXF + 5) / 100;
      if (oldCannonX != cannonX) {
        // the cannon moved
        // erase it
        drawCannon(oldCannonX, CANNON_Y, 0);
        oldCannonX = cannonX;
        // draw it
        drawCannon(cannonX, CANNON_Y, 1);
        clock += 60 / speedAdjust;
      }
      return true;
    }
  }
  return input;
}


void tick() {
  playTone(tickFreq[tickFreqCounter++], 60, 1);
  tickFreqCounter = tickFreqCounter % 4;
}



void playTone(unsigned int frequency, unsigned long duration_ms) {
  // Default is to play tone with highest priority.
  playTone(frequency, duration_ms, 9);
}

void playTone(unsigned int frequency, unsigned long duration_ms, byte priority) {
  // priority is value 0-9, 9 being highest priority
  if (TCCR2B > 0) {
    // If a tone is currently playing, check priority
    if (priority < currentTonePriority) {
      return;
    }
  }
  currentTonePriority = priority;
  tv.tone(frequency, duration_ms);
}

void incCoinCounter() {
  if (lastCoinTime + 300 > millis()) {
    return;
  }
  // инкрементируем счётчик количества рублей
  coinsCount++;
  lastCoinTime = millis();
}
