void menuNavigate() {
  // note that the call to pollFireButton above got data from the nunchuk device
  if ((Controller.upPressed()) || (useNunchuk && (Nunchuk.getJoystickY() > 200))) {
    menuChoise--;
    if (menuChoise < 3) {
      menuChoise = 3;
    } else {
      playTone(1046, 20);
      switchArrow(menuChoise);
    }
  }
  if ((Controller.downPressed()) || (useNunchuk && (Nunchuk.getJoystickY() < 100))) {
    menuChoise++;
    if (menuChoise > 4) {
      menuChoise = 4;
    } else {
      playTone(1046, 20);
      switchArrow(menuChoise);
    }
  }
}

boolean getInput() {
  boolean input = false;

  if (useNunchuk) {
    Nunchuk.getData();
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
