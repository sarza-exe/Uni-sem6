void handleAutomaticMode(){
  // Jeśli jedziemy, sprawdzaj odległość
  if(carState == MOVE) {
    if(millis() - lastSonarCheck >= sonarInterval) {
      lastSonarCheck = millis();
      unsigned int sonarCheck = GetSonarDistance();
      
      if(sonarCheck > 0 && sonarCheck <= 40) { // Dodałem > 0, bo HC-SR04 czasem zwraca 0 przy błędzie
        SetState(SCAN); // Przeszkoda. Zatrzymaj się i skanuj
      }
    }
  }
  
  // Jeśli jesteśmy w trybie skanowania, wywołuj funkcję radaru
  if(carState == SCAN) {
    PerformScan();
  }

  // Jeśli autko skończyło skręcać, ruszaj znowu przed siebie
  if(carState == TURN_WAIT) {
    // Kiedy turnLeft/turnRight zliczy impulsy z kół, ustawia movement = NONE. 
    // To dla nas sygnał, że skręt zakończony!
    if(movement == NONE) {
      SetState(MOVE); 
    }
  }
}

void handleManualMode(uint32_t cmd) {
  int dist = GetSonarDistance();

  // Jeśli jedziemy, a od ostatniego rozkazu minęło więcej niż 200ms - STÓJ
  if (movement != NONE && (millis() - lastManualCommandTime > 200)) {
    w.stop();
    movement = NONE;
    Serial.println("Auto-Stop: Brak sygnału");
  }

  if (movement == FORWARD && dist > 0 && dist < 15) {
    w.stop();
    movement = NONE;
    return;
  }

  if (cmd != IR_ERR) {
    // Jeśli przyszedł jakikolwiek poprawny kod sterujący, odświeżamy czasomierz
    lastManualCommandTime = millis();

    switch(cmd) {
      case IR_UP:
        // Dodajemy dist == 0, bo sonar zwraca 0, gdy przed nim jest pusto (daleko)
        if (dist > 15 || dist == 0) { 
          w.setSpeed(175);
          w.forward();
          movement = FORWARD;
        }
        break;
        
      case IR_DOWN:
        w.setSpeed(175);
        w.back();
        movement = BACKWARD;
        break;

      case IR_LEFT:
        w.setSpeed(175);
        w.forwardLeft();
        w.backRight();
        movement = TURN;
        break;

      case IR_RIGHT:
        w.setSpeed(175);
        w.backLeft();
        w.forwardRight();
        movement = TURN;
        break;

      case IR_STAR:
        w.stop();
        movement = NONE;
        break;
    }
  }
}