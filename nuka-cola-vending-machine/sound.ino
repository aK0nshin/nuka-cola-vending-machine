void welcomeTune() {
  playTone(1046, 20);
  tv.delay(16);
  playTone(1318, 20);
  tv.delay(16);
  playTone(1568, 20);
  tv.delay(16);
  playTone(2093, 20);
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
