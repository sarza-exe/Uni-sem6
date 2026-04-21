#include "Wheels.h"
#include "Display.h"
//#include "TimerOne.h"
#include "PinChangeInterrupt.h"
#include <Servo.h>

// NIE BRAC SAMOCHODZIKOW 0 I 2 
// SAMOCHÓD NUMER 20 COŚ NIE STYKA

#define INTINPUT0 A0
#define INTINPUT1 A1
#define BEEPER 13

// piny dla sonaru (HC-SR04)
#define TRIG A2
#define ECHO A3

// pin kontroli serwo (musi być PWM)
#define SERVO 3
Servo serwo;

long int intPeriod = 500000;

volatile unsigned long cnt0, cnt1;

Wheels w;
Display d;
volatile char cmd;

unsigned long moveStart = 0;
unsigned long moveTime = 0;
unsigned long totalMoveCount = 0;

unsigned long lastDisplayUpdate = 0;
unsigned long lastSonarCheck = 0;
const unsigned long displayInterval = 150; // Odświeżaj co 150ms
const unsigned long sonarInterval = 200; // Odświeżaj co 50ms


// = #szczelin / (3.14*średnica koła np 6.5)
const float PULSES_PER_CM = 1.85;

enum movement_enum{
  NONE = 0,
  FORWARD = 1,
  BACKWARD = 2,
  TURN = 3,
};

enum car_state{
  MOVE = 0,
  SCAN = 1,
  TURN_WAIT = 2, // czekamy aż autko fizycznie skończy obrót
};

enum scan_state{
  SCAN_START = 0,
  SCAN_WAIT_LEFT = 1,
  SCAN_WAIT_RIGHT = 2,
  SCAN_DECIDE = 3,
};

scan_state scanPhase = SCAN_START;
unsigned long scanTimer = 0;
int distLeft = 0;
int distRight = 0;


int currentSpeedL = 0;
int currentSpeedR = 0;

movement_enum movement;
car_state carState = 0;

int totalDistance = 0;

/*
Napisz program, który pozwoli autku:
    wyświetlać dane z sonaru (kąt patrzenia, odległość do przeszkody) na ekranie LCD;
    zatrzymywać się przed przeszkodą;
    podejmować decyzję co do sposobu jej ominięcia;
    kontynuować jazdę . 
*/

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);    // TRIG startuje sonar
  pinMode(ECHO, INPUT);     // ECHO odbiera powracający impuls
  serwo.attach(SERVO);
  serwo.write(90);

  pinMode(BEEPER, OUTPUT);

  // put your setup code here, to run once:
  w.attach(8,7,5,11,12,6);

  pinMode(INTINPUT0, INPUT);
  pinMode(INTINPUT1, INPUT);

  cnt0=0;
  cnt1=0;

  attachPCINT(digitalPinToPCINT(INTINPUT0), increment, CHANGE);
  attachPCINT(digitalPinToPCINT(INTINPUT1), increment, CHANGE);
  
  Serial.begin(9600);
  Serial.println("Forward: WAD");
  Serial.println("Back: ZXC");
  Serial.println("Stop: S");

  // delay(2000); w.setSpeed(175); turnLeft(180); goForward(100);

  d.init();
  SetState(MOVE);
}

void loop() {
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
      case '5': currentSpeedL = 175; currentSpeedR = 175; w.setSpeed(175); break;
      case '6': currentSpeedL = 125; currentSpeedR = 125; w.setSpeed(125); break;
      case 'u': goForward(10); break;
      case 'j': goBack(10); break;
      case 'i': goForward(25); break;
      case 'k': goBack(100); break;
      case 'y': turnRight(90); break;
      case 't': turnLeft(90); break;
      case '3': serwo.write(45); break;
      case '4': serwo.write(135); break;
    }
  }

  if (movement != NONE) {
    //unsigned long elapsed = millis() - moveStart;
    unsigned long elapsedCnt = (cnt0 > cnt1) ? cnt0 : cnt1;

    //if (elapsed >= moveTime) {
    if (elapsedCnt >= totalMoveCount) {
      w.stop();
      movement = NONE;
      //TimerOff();
      d.updateDashboard(0, 0, 0, movement);
    } 
    else {
      // Wywołuj odświeżanie tylko raz na 150ms
      if (millis() - lastDisplayUpdate >= displayInterval) {
        //int remaining = totalDistance - (elapsed / 25);
        // remaining = totalDistance * (1 - elapsedCnt / totalMoveCount) wspolny mianownik
        int remaining = (int)(((unsigned long)(totalMoveCount - elapsedCnt) * totalDistance) / totalMoveCount);
        d.updateDashboard(remaining, currentSpeedL, currentSpeedR, movement);
        lastDisplayUpdate = millis();
      }
    }
  }
  
  // Jeśli jedziemy, sprawdzaj odległość
  if(carState == MOVE) {
    if(millis() - lastSonarCheck >= sonarInterval) {
      lastSonarCheck = millis();
      unsigned int sonarCheck = GetSonarDistance();
      
      if(sonarCheck > 0 && sonarCheck <= 40) { // Dodałem > 0, bo HC-SR04 czasem zwraca 0 przy błędzie
        SetState(SCAN); // Przeszkoda! Zatrzymaj się i skanuj
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



unsigned int GetSonarDistance(){
  unsigned long tot;      // czas powrotu (time-of-travel)
  unsigned int distance;
  
/* uruchamia sonar (puls 10 ms na `TRIGGER')
 * oczekuje na powrotny sygnał i aktualizuje
 */
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(12); //microseconds
  digitalWrite(TRIG, LOW);
  tot = pulseIn(ECHO, HIGH);

/* prędkość dźwięku = 340m/s => 1 cm w 29 mikrosekund
 * droga tam i z powrotem, zatem:
 */
  distance = tot/58;
  return distance;
}




void SetState(car_state newState){
  if(carState == newState) return; 

  if(newState == MOVE){
    serwo.write(90);
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
    d.updateDashboard(0, 0, 0, movement);
  }
  else if(newState == TURN_WAIT){
    carState = TURN_WAIT;
    // Nie wywołujemy w.stop(), bo turnLeft/turnRight same zarządzają silnikami
  }
}




void PerformScan() {
  switch (scanPhase) {
    
    case SCAN_START:
      serwo.write(160); // Patrz w lewo (160 stopni)
      scanTimer = millis();
      scanPhase = SCAN_WAIT_LEFT; // Przejdź do następnego kroku
      break;

    case SCAN_WAIT_LEFT:
      if (millis() - scanTimer >= 350) { // Czekamy 350ms na ruch serwa
        distLeft = GetSonarDistance();
        Serial.print("Lewo: "); Serial.println(distLeft);
        
        serwo.write(20); // Patrz w prawo (20 stopni)
        scanTimer = millis();
        scanPhase = SCAN_WAIT_RIGHT;
      }
      break;

    case SCAN_WAIT_RIGHT:
      if (millis() - scanTimer >= 400) { // Czekamy 400ms (dłuższy ruch przez cały przód)
        distRight = GetSonarDistance();
        Serial.print("Prawo: "); Serial.println(distRight);
        
        serwo.write(90); // Wróć na środek
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



// zmienia wartość pinu BEEPER
void doBeep() {
  digitalWrite(BEEPER, digitalRead(BEEPER) ^ 1);
}



void goForward(int cm) {
  //moveTime = cm * 25;      // czas ruchu
  //totalMoveCount = cm * currentSpeedL / 110;
  totalMoveCount = (unsigned long)(cm * PULSES_PER_CM);
  totalDistance = cm;

  moveStart = millis();
  resetCount();
  movement = FORWARD;

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.forward();
}



void goBack(int cm) {
  //moveTime = cm * 25;      // czas ruchu
  totalMoveCount = (unsigned long)(cm * PULSES_PER_CM);
  totalDistance = cm;

  moveStart = millis();
  resetCount();
  movement = BACKWARD;

  intPeriod = 3000L * currentSpeedL; 
  //TimerUpdate();

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.back();
}



void turnLeft(int degrees){
  float distanceToTravel = (degrees / 360.0) * (105); // change 31 to PI * odległość między środkami kół
  totalMoveCount = (unsigned long)(distanceToTravel * PULSES_PER_CM);

  resetCount();
  movement = FORWARD; //TODO nowy typ TURN

  d.updateDashboard(degrees, currentSpeedL, currentSpeedR, movement);
  w.forwardLeft();
  w.backRight();
}

void turnRight(int degrees){
  float distanceToTravel = (degrees / 360.0) * (105); // change 31 to PI * odległość między środkami kół
  totalMoveCount = (unsigned long)(distanceToTravel * PULSES_PER_CM);

  resetCount();
  movement = FORWARD; //TODO nowy typ TURN + remainingDegrees in loop

  d.updateDashboard(degrees, currentSpeedL, currentSpeedR, movement);
  w.backLeft();
  w.forwardRight();
}

void resetCount(){
  cnt0 = 0;
  cnt1 = 0;
}

void increment() {
  if(digitalRead(INTINPUT0))
    cnt0++;
  if(digitalRead(INTINPUT1))
    cnt1++;
}