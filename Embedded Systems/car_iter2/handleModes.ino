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





void handleSpringMode() {
  const int targetDist = 45; // Punkt równowagi (cm)
  const float k = 4.0; // Współczynnik sprężystości (do dostosowania)
  const int deadzone = 3; // Tolerancja błędu (żeby auto nie drgało przy 100cm)

  int currentDist = GetSonarDistance();

  // Ignorujemy błędy sonaru (0 lub zbyt duże wartości)
  if (currentDist <= 0 || currentDist > 120) {
    w.setSpeed(150);
    w.back();
    return;
  }

  int x = currentDist - targetDist; // Obliczamy "rozciągnięcie" sprężyny (x)

  // czy jesteśmy w punkcie równowagi
  if (abs(x) <= deadzone) {
    w.stop();
    movement = NONE;
  } 
  else {
    // Obliczamy prędkość na podstawie siły sprężyny (F = k * x) Mapujemy wynik na zakres PWM (0-255)
    int speed = abs(x) * k;
    
    // Ograniczamy prędkość do zakresu obsługiwanego przez silniki
    if (speed > 255) speed = 255;
    if (speed < 100)  speed = 100; // Minimalna moc, by ruszyć silniki

    w.setSpeed(speed);

    if (x > 0) {
      // Przeszkoda daleko
      w.forward();
      movement = FORWARD;
    } 
    else {
      // Przeszkoda za blisko
      Serial.println("ZA BLISKO");
      w.back();
      movement = BACKWARD;
    }
  }
}

