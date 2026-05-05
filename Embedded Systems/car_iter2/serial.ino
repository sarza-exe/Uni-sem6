void performSerialRead(){

  while(Serial.available())
  {
    cmd = Serial.read();
    switch(cmd)
    {
      case 'w': w.forward(); break;
      case 'x': w.back(); intPeriod = 50000*currentSpeedL; /*TimerUpdate();*/ break;
      case 'a': w.forwardLeft(); break;
      case 'd': w.forwardRight(); break;
      case 'z': w.backLeft(); break;
      case 'c': w.backRight(); break;
      case 's': w.stop(); movement = NONE; d.updateDashboard(0, 0, 0, movement);//TimerOff(); break;
      case '1': currentSpeedL = 100; w.setSpeedLeft(100); break;
      case '2': currentSpeedL = 200; w.setSpeedLeft(200); break;
      case '9': currentSpeedR = 100; w.setSpeedRight(100); break;
      case '0': currentSpeedR = 200; w.setSpeedRight(200); break;
      case '7': w.setSpeed(0, 180); break;
      case '5': currentSpeedL = 175; currentSpeedR = 175; w.setSpeed(175); break;
      case '6': currentSpeedL = 125; currentSpeedR = 125; w.setSpeed(125); break;
      case 'u': goForward(10); break;
      case 'j': goBack(10); break;
      case 'i': goForward(25); break;
      case 'k': goBack(100); break;
      case 'y': turnRight(90); break;
      case 't': turnLeft(90); break;
      case '3': serwoWrite(45); break;
      case '4': serwoWrite(135); break;
    }
  }

}