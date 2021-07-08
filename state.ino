void switchState(byte newState) {
  state = newState;
  delay(1000);

  Serial.print(F("newState: "));
  Serial.println(newState);
}

void welcomeLoop() {
  if (coinsCount < 2) {
    return;
  }
  coinsCount = coinsCount - 2;
  switchState(2);
}
