byte menuChoice(byte nChoices, byte *choices) {
  char choice = 0;
  tv.fill(0);
  byte x = 24;
  byte y;

  while (true) {
    for (byte i = 0; i < nChoices; i++) {
      strcpy_P(s, (char *)pgm_read_word(&(strings[choices[i]])));
      tv.print(32, 30 + (i * 8), s);
    }
    for (byte i = 0; i < nChoices; i++) {
      y = 30 + (i * 8);
      if (i == choice) {
        // draw arrow next to selected game
        drawArrow(x, y);
      } else {
        for (byte j = 0; j < 8; j++) {
          tv.draw_line(x, y + j, x + 7, y + j, 0);
        }
      }
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

boolean titleScreen() {
  byte x, y;
  int d;

  tv.fill(0);
  tv.delay(32);
  tv.select_font(font6x8);

  d = 10;

  for (x = 1; x <= 3; x++) {
    strcpy_P(s, (char *)pgm_read_word(&(strings[1])));
    // cleverness to make the letters appear one at a time.
    // truncate the string with a null character.
    s[x] = '\0';
    tv.print(56, 20, s);
    if (pollFireButton(d)) {
      return true;
    }
  }
  drawBitmap(74, 20, BITMAP_INVERTED_Y);
  if (pollFireButton(d)) {
    return true;
  }

  for (x = 1; x <= 14; x++) {
    if (x == 6) continue; // don't delay on the space char
    strcpy_P(s, (char *)pgm_read_word(&(strings[2])));
    s[x] = '\0';
    tv.print(28, 30, s);
    if (pollFireButton(d)) {
      return true;
    }
  }

  x = 74;

  tv.select_font(font4x6);

  d = 60;
  if (pollFireButton(d)) {
    return true;
  }

  y = 49;
  drawBitmap(48, y, BITMAP_MYSTERY_SHIP);
  strcpy_P(s, (char *)pgm_read_word(&(strings[6])));
  tv.print(64, y, s);
  if (pollFireButton(d)) {
    return true;
  }


  y = 58;
  drawBitmap(47, y, 0);
  strcpy_P(s, (char *)pgm_read_word(&(strings[5])));
  tv.print(64, y + 1, s);
  if (pollFireButton(d)) {
    return true;
  }


  y = 68;
  drawBitmap(48, y, 16);
  strcpy_P(s, (char *)pgm_read_word(&(strings[4])));
  tv.print(64, y + 1, s);

  if (pollFireButton(d)) {
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
    if (pollFireButton(2)) {
      return true;
    }
    x--;
    drawBitmap(x, y, BITMAP_BLANK);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(160);

  for (x = 80; x < W + 5; x++) {
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 0);
    if (pollFireButton(2)) {
      return true;
    }
    x++;
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(320);

  for (x = W + 5; x > 79; x--) {
    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y + 8);
    drawBitmap(x, y, 0);
    if (pollFireButton(2)) {
      return true;
    }
    x--;

    drawBitmap(x - 6, y + 1, BITMAP_BLANK);
    drawBitmap(x - 1, y, BITMAP_BLANK);
    drawBitmap(x - 5, y + 1, BITMAP_INVERTED_Y + 8);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(500);
  drawBitmap(80, y, BITMAP_BLANK);

  if (pollFireButton(120)) {
    return true;
  }
  return false;
}
