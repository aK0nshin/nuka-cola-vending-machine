void switchState(byte newState) {
  state = newState;
  tv.delay(160);

  Serial.print(F("newState: "));
  Serial.println(newState);
}

void toMenu() {
  switchState(2);
  menuChoice = 3;
  drawMenu();
}

void toBottle() {
  switchState(3);
  drawBottle();
}

void toGame() {
  switchState(4);
  initSpaceInvaders(false);
}

void welcomeLoop() {
  if (coinsCount < 2) {
    return;
  }
  coinsCount = coinsCount - 2;
  toMenu();
}

void menuLoop() {
  // get input
  if (pollFireButton(10)) {
    playTone(1046, 20);

    switch (menuChoise) {
      case 3: // Бутылка
        toBottle();
        break;
      case 4: // Игра
        toGame();
        break;
    }
  }
  menuNavigate();
}

void bottleLoop() {
  
}

void gameLoop() {
  spaceInvaders();
}
