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
