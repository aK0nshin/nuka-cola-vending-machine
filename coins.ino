void incCoinCounter() {
  // инкрементируем счётчик количества рублей
  coinsCount++;
  // выводим текущий баланс
  Serial.print(F("coinsCount: "));
  Serial.println(coinsCount);
}
