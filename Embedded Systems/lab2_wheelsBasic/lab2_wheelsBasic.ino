#include "Wheels.h"
#include "Display.h"
#include "TimerOne.h"
#include "PinChangeInterrupt.h"

#define INTINPUT0 A0
#define INTINPUT1 A1
#define BEEPER 13

long int intPeriod = 500000;

volatile int cnt0, cnt1;

Wheels w;
Display d;
volatile char cmd;

unsigned long moveStart = 0;
unsigned long moveTime = 0;
unsigned long moveCount = 0;

unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 150; // Odświeżaj co 150ms

enum movement_enum{
  NONE = 0,
  FORWARD = 1,
  BACKWARD = 2,
};
int currentSpeedL = 0;
int currentSpeedR = 0;

movement_enum movement;

int totalDistance = 0;

void setup() {
  pinMode(BEEPER, OUTPUT);
  Timer1.initialize();
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

  d.init();

}

void loop() {
  while(Serial.available())
  {
    cmd = Serial.read();
    switch(cmd)
    {
      case 'w': w.forward(); break;
      case 'x': w.back(); intPeriod = 50000*currentSpeedL; TimerUpdate(); break;
      case 'a': w.forwardLeft(); break;
      case 'd': w.forwardRight(); break;
      case 'z': w.backLeft(); break;
      case 'c': w.backRight(); break;
      case 's': w.stop(); movement = NONE; d.updateDashboard(0, 0, 0, movement); TimerOff(); break;
      case '1': currentSpeedL = 100; w.setSpeedLeft(100); break;
      case '2': currentSpeedL = 200; w.setSpeedLeft(200); break;
      case '9': currentSpeedR = 100; w.setSpeedRight(100); break;
      case '0': currentSpeedR = 200; w.setSpeedRight(200); break;
      case '5': currentSpeedL = 150; currentSpeedR = 150; w.setSpeed(150); break;
      case 'u': forwardNew(10); break;
      case 'j': backNew(10); break;
      case 'o': forwardNew(25); break;
      case 'p': backNew(250); break;
    }
  }

  if (movement != NONE) {
    unsigned long elapsed = millis() - moveStart;
    unsigned long elapsedCnt = cnt0;
    if(cnt1 > cnt0) elapsedCnt = cnt1;

    //if (elapsed >= moveTime) {
    if (elapsedCnt >= moveCount) {
      w.stop();
      movement = NONE;
      TimerOff();
      d.updateDashboard(0, 0, 0, movement);
    } 
    else {
      // Wywołuj odświeżanie tylko raz na 150ms!
      if (millis() - lastDisplayUpdate >= displayInterval) {
        int remaining = totalDistance - (elapsed / 25);
        d.updateDashboard(remaining, currentSpeedL, currentSpeedR, movement);
        lastDisplayUpdate = millis();
      }
    }
  }
}

// aktualizuje Timer1 aktualną wartością intPeriod
void TimerUpdate() {
  Timer1.detachInterrupt(); //odłączamy by procesor nie wywoływał starej funkcji
  Timer1.attachInterrupt(doBeep, intPeriod); //Co każde intPeriod mikrosekund przerwij wszystko, co robisz (nawet jeśli jesteś w środku pętli loop), skocz do funkcji doBeep, wykonaj ją błyskawicznie i wróć do pracy
}

void TimerOff(){
  Timer1.detachInterrupt();
  digitalWrite(BEEPER, LOW); 
}

// zmienia wartość pinu BEEPER
void doBeep() {
  digitalWrite(BEEPER, digitalRead(BEEPER) ^ 1);
}

void forwardNew(int cm) {
  resetCount();
  moveTime = cm * 25;      // czas ruchu
  moveCount = cm * currentSpeedL / 110;
  totalDistance = cm;

  moveStart = millis();
  movement = FORWARD;

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.forward();
}

void backNew(int cm) {
  resetCount();
  moveTime = cm * 25;      // czas ruchu
  moveCount = cm * currentSpeedL / 110;
  totalDistance = cm;

  moveStart = millis();
  movement = BACKWARD;

  intPeriod = 6000L * currentSpeedL; 
  TimerUpdate();

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.back();
}

void resetCount(){
  cnt0 = 0;
  cnt1 = 0;
}

void increment() {
  if(digitalRead(INTINPUT0))
    cnt0++;
  else if(digitalRead(INTINPUT1))
    cnt1++;
}