void updateMovement(){
  if (movement != NONE) {
    //unsigned long elapsed = millis() - moveStart;
    unsigned long elapsedCnt = (cnt0 > cnt1) ? cnt0 : cnt1;

    //if (elapsed >= moveTime) {
    if (elapsedCnt >= totalMoveCount) {
      w.stop();
      movement = NONE;
      //TimerOff();
      //d.updateDashboard(0, 0, 0, movement);
    } 
    else {
      // Wywołuj odświeżanie tylko raz na 150ms
      if (millis() - lastDisplayUpdate >= displayInterval) {
        //int remaining = totalDistance - (elapsed / 25);
        // remaining = totalDistance * (1 - elapsedCnt / totalMoveCount) wspolny mianownik
        int remaining = (int)(((unsigned long)(totalMoveCount - elapsedCnt) * totalDistance) / totalMoveCount);
        //d.updateDashboard(remaining, currentSpeedL, currentSpeedR, movement);
        lastDisplayUpdate = millis();
      }
    }
  }
}

void SetState(car_state newState){
  if(carState == newState) return; 

  if(newState == MOVE){
    serwoWrite(90);
    carState = MOVE;
    movement = NONE; 
    w.setSpeed(150); 
    w.forward();
  }
  else if(newState == SCAN){
    carState = SCAN;
    w.stop(); 
    movement = NONE; 
    scanPhase = SCAN_START; 
    //d.updateDashboard(0, 0, 0, movement);
  }
  else if(newState == TURN_WAIT){
    carState = TURN_WAIT;
    // Nie wywołujemy w.stop(), bo turnLeft/turnRight same zarządzają silnikami
  }
  else if(newState == STAY){
    carState = STAY;
    w.setSpeed(0);
    movement = NONE;
  }
}




void PerformScan() {
  switch (scanPhase) {
    
    case SCAN_START:
      serwoWrite(160);
      scanTimer = millis();
      scanPhase = SCAN_WAIT_LEFT; // Przejdź do następnego kroku
      break;

    case SCAN_WAIT_LEFT:
      if (millis() - scanTimer >= 350) { // Czekamy 350ms na ruch serwa
        distLeft = GetSonarDistance();
        Serial.print("Lewo: "); Serial.println(distLeft);
        
        serwoWrite(20);
        scanTimer = millis();
        scanPhase = SCAN_WAIT_RIGHT;
      }
      break;

    case SCAN_WAIT_RIGHT:
      if (millis() - scanTimer >= 400) { // Czekamy 400ms (dłuższy ruch przez cały przód)
        distRight = GetSonarDistance();
        Serial.print("Prawo: "); Serial.println(distRight);
        
        serwoWrite(90);
        scanTimer = millis();
        scanPhase = SCAN_DECIDE;
      }
      break;

    case SCAN_DECIDE:
      if (millis() - scanTimer >= 300) { // Czekamy aż głowa wróci na środek
        // decyzja
        if (distLeft > distRight) {
          Serial.println("Decyzja: Skret w LEWO");
          w.setSpeed(200);
          turnLeft(90); 
        } else {
          Serial.println("Decyzja: Skret w PRAWO");
          w.setSpeed(200);
          turnRight(90);
        }
        
        movement = TURN;
        scanPhase = SCAN_START; // Resetujemy maszynę skanującą na przyszłość
        SetState(TURN_WAIT);    // Zmieniamy stan auta - teraz czeka na koniec obrotu!
      }
      break;
  }
}