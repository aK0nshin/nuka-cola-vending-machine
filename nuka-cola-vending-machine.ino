#include <avr/pgmspace.h>
#include <TVout.h>
#include <video_gen.h>
#include "font4x6.h"
#include <font6x8.h>
#include <EEPROM.h>
#include <Controllers.h>

// ========== PINS ==========
//#define LEFT_BUTTON 3
//#define RIGHT_BUTTON 2
//#define UP_BUTTON 4
//#define DOWN_BUTTON 5
//#define FIRE_BUTTON 10   Зачем это всё?
#define COIN_ACCEPTOR_PIN 10
#define TV_MISO_PIN 12

// SETTINGS 
#define W 136
#define H 98
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
#define LASER_DRAW_INTERVAL 75
#define BOMB_DRAW_INTERVAL 300
#define MYSTERY_SHIP_WIDTH 11
#define MYSTERY_SHIP_DRAW_INTERVAL 300
#define MYSTERY_SHIP_INTERVAL 75000
#define BITMAP_MYSTERY_SHIP 48
#define BITMAP_EXPLOSION 56
#define BITMAP_BLANK 64
#define BITMAP_INVERTED_Y 104
#define INIT_INVADER_ADVANCE_INTERVAL 3000

const uint16_t bitmaps[] PROGMEM = {
#include "bitmaps.h"
};

const char welcome0[] PROGMEM = "NUKA-COLA";
const char welcome1[] PROGMEM = "Vending Machine";
const char welcome2[] PROGMEM = "Insert 2 Coins";
const char welcome3[] PROGMEM = "Coins in: ";
const char* const welcome[] PROGMEM = {welcome0, welcome1, welcome2, welcome3};

const char menu0[] PROGMEM = "NUKA-COLA";
const char menu1[] PROGMEM = "vs";
const char menu2[] PROGMEM = "SPACE INVADERS";
const char menu3[] PROGMEM = "Play game";
const char menu4[] PROGMEM = "Get bottle";
const char* const menu[] PROGMEM = {menu0, menu1, menu2, menu3, menu4};

const char s0[] PROGMEM = "HIGH SCORES";
const char s1[] PROGMEM = "PLAY GAME";
const char s2[] PROGMEM = "SPACE INVADERS";
const char s3[] PROGMEM = " =  10";
const char s4[] PROGMEM = " =  20";
const char s5[] PROGMEM = " =  30";
const char s6[] PROGMEM = " =  ??";
const char s7[] PROGMEM = "GAME";
const char s8[] PROGMEM = "OVER";
const char s9[] PROGMEM = "VAULT-TEC";
const char s10[] PROGMEM = "123456789012345678";
const char s11[] PROGMEM = "qwertyuiopasdfghjk";
const char s12[] PROGMEM = "18 bukv ili cifr..";
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

TVout tv;

byte state;
byte coinsCount;
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
  Serial.begin(9600);
  while (!Serial);

  // If pin 12 is pulled LOW, then the PAL jumper is shorted.
  pinMode(COIN_ACCEPTOR_PIN, INPUT);
  pinMode(TV_MISO_PIN, INPUT);
  digitalWrite(TV_MISO_PIN, HIGH);

  if (digitalRead(TV_MISO_PIN) == LOW) {
    tv.begin(_PAL, W, H);
    // Since PAL processing is faster, we need to slow the game play down.
    speedAdjust = 1.4;
  } else {
    tv.begin(_NTSC, W, H);
  }
  attachInterrupt(COIN_ACCEPTOR_PIN, incCoinCounter, RISING);

  coinsCount = 0;
  state = 1;
  tv.select_font(font6x8);
  randomSeed(analogRead(0));

  welcomeTune();

  // Detect whether nunchuk is connected.  Poll the nunchuk every 4th frame.
  useNunchuk = Nunchuk.init(tv, 4);
  if (useNunchuk) {
    // Speed up game play a bit because of the extra time it takes to
    // communicate with the nunchuk.
    speedAdjust *= 0.8;
  }

  byte m[2] = {2, 9};
  byte choice = menuChoice(2, m);
  if (choice == 0) {
    tv.delay(160);
    initSpaceInvaders(false);
  }
}

void loop() {
  switch (state) {
    case 1: // Страница приветствия
      drawWelcome();
      break;
    case 2: // Страница выбора игра/бутылка

      break;
    case 3: // Страница игры
      spaceInvaders();
      break;
    case 4: // Страница секретная

      break;
    default: 
      switchState(1);
  }
}
