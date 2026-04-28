#include <IRremote.hpp>

#include "Wheels.h"
#include "Display.h"
//#include "TimerOne.h"
#include "PinChangeInterrupt.h"
#include <Servo.h>

// NIE BRAC SAMOCHODZIKOW 0 I 2 
// SAMOCHÓD NUMER 20 COŚ NIE STYKA
// SAMOCHODZIK 5 FAJNY
// Brać granatowy pilot

#define INTINPUT0 A0
#define INTINPUT1 A1
#define BEEPER 13

// piny dla sonaru (HC-SR04)
#define TRIG A2
#define ECHO A3

// pin pilota IR
#define IR_RECEIVE_PIN 3 

//kody komend do pilota
#define IR_UP 24
#define IR_DOWN 82
#define IR_LEFT 8
#define IR_RIGHT 90
#define IR_ONE 69
#define IR_TWO 70
#define IR_THREE 71
#define IR_STAR 22
#define IR_ERR 1 // nothing

uint32_t lastCommand = IR_ERR;

// pin kontroli serwo (musi być PWM)
#define SERVO 9
Servo serwo;

int serwoAngle = 90;

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

unsigned long lastManualCommandTime = 0;


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
  STAY = 3,
};

enum scan_state{
  SCAN_START = 0,
  SCAN_WAIT_LEFT = 1,
  SCAN_WAIT_RIGHT = 2,
  SCAN_DECIDE = 3,
};

enum op_mode { MODE_NONE, MODE_AUTO, MODE_MANUAL };
op_mode currentOpMode = MODE_NONE;

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
TODO
add IR_TWO IR_THREE
2 przyciski
"2" to porusza się jak wcześniej
"3" to porusza się jak się przytrzyma strzałki; strzałki działąją i jak strzałka w przód + wjechanie w ściane to się zatrzyma
*/

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);    // TRIG startuje sonar
  pinMode(ECHO, INPUT);     // ECHO odbiera powracający impuls
  serwo.attach(SERVO);
  serwoWrite(90);

  pinMode(BEEPER, OUTPUT);

  // put your setup code here, to run once:
  w.attach(8,7,5,11,12,6);

  pinMode(INTINPUT0, INPUT);
  pinMode(INTINPUT1, INPUT);

  cnt0=0;
  cnt1=0;

  attachPCINT(digitalPinToPCINT(INTINPUT0), increment, CHANGE);
  attachPCINT(digitalPinToPCINT(INTINPUT1), increment, CHANGE);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.begin(9600);
  Serial.println("Forward: WAD");
  Serial.println("Back: ZXC");
  Serial.println("Stop: S");

  d.init();

  GetCommandFromIR(IR_ONE);
  GetCommandFromIR(IR_STAR);
  Serial.println("CAR UNLOCKED");

  SetState(STAY); //STAY or MOVE
}

void loop() {
  performSerialRead();

  uint32_t irCmd = ReadCommandFromIR();

  //d.serwoDisplay(serwoAngle, GetSonarDistance());
  if (millis() - lastDisplayUpdate >= displayInterval) {
    d.serwoDisplay(serwoAngle, GetSonarDistance());
    lastDisplayUpdate = millis();
  }

  // Tryby
  if (irCmd == IR_TWO) {
    w.setSpeed(175);
    SetState(MOVE);
    currentOpMode = MODE_AUTO;
    Serial.println("Tryb: AUTO");
  } 
  else if (irCmd == IR_THREE) {
    serwoWrite(90);
    w.setSpeed(175);
    currentOpMode = MODE_MANUAL;
    w.stop(); // Zatrzymaj się przy zmianie trybu
    Serial.println("Tryb: MANUAL");
  }
  else if(irCmd == IR_STAR){
    currentOpMode = MODE_NONE;
    w.stop();
    Serial.println("Tryb: STOP");
  }

  // Wykonaj logikę zależną od trybu
  if (currentOpMode == MODE_AUTO) {
    updateMovement();
    handleAutomaticMode(); // logika z sonarem i skanowaniem
  } 
  else if (currentOpMode == MODE_MANUAL) {
    handleManualMode(irCmd); // funkcja sterowania strzałkami
  }
  
}


void GetCommandFromIR(uint32_t command){
  uint32_t commandReceived = IR_ERR;
  while(true){
    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.decodedRawData != 0) {
        
        Serial.print("Odebrano kod przycisku: 0x");
        Serial.print(command);
        Serial.print(" ?=  0x");
        // Wyświetlamy kod w formacie szesnastkowym (HEX)
        commandReceived = IrReceiver.decodedIRData.command;
        Serial.println(commandReceived);
      }
      // Wznów nasłuchiwanie, aby odebrać kolejny sygnał
      IrReceiver.resume(); 
      if(commandReceived == command) break;
    }
  }
}


// uint32_t ReadCommandFromIR(){
//   uint32_t command = IR_ERR;
//   if (IrReceiver.decode()) {
//     if (IrReceiver.decodedIRData.decodedRawData != 0) {
//       Serial.print("Odebrano kod przycisku: 0x");
//       command = IrReceiver.decodedIRData.command;
//       Serial.println(command);
//     }
//     // Wznów nasłuchiwanie, aby odebrać kolejny sygnał
//     IrReceiver.resume(); 
//     return command;
//   }
//   return IR_ERR;
// }

uint32_t ReadCommandFromIR() {
  if (IrReceiver.decode()) {
    uint32_t cmd = IrReceiver.decodedIRData.command;
    
    // Jeśli to kod powtórzenia, zwróć ostatnią zapamiętaną komendę
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
        IrReceiver.resume();
        return lastCommand; 
    }

    Serial.print("IRcommand: ");
    Serial.println(cmd);
    
    IrReceiver.resume();
    lastCommand = cmd;
    return cmd;
  }
  return IR_ERR;
}



unsigned int GetSonarDistance(){
  unsigned long tot;      // czas powrotu (time-of-travel)
  unsigned int distance;
  
// uruchamia sonar (puls 10 ms na `TRIGGER') oczekuje na powrotny sygnał aktualizuje
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(12); //microseconds
  digitalWrite(TRIG, LOW);
  tot = pulseIn(ECHO, HIGH);

// prędkość dźwięku = 340m/s => 1 cm w 29 mikrosekund droga tam i z powrotem zatem:
  distance = tot/58;
  return distance;
}



void turnLeft(int degrees){
  float distanceToTravel = (degrees / 360.0) * (105); // change 31 to PI * odległość między środkami kół
  totalMoveCount = (unsigned long)(distanceToTravel * PULSES_PER_CM);

  resetCount();
  movement = FORWARD; //TODO nowy typ TURN

  //d.updateDashboard(degrees, currentSpeedL, currentSpeedR, movement);
  w.forwardLeft();
  w.backRight();
}


void turnRight(int degrees){
  float distanceToTravel = (degrees / 360.0) * (105); // change 31 to PI * odległość między środkami kół
  totalMoveCount = (unsigned long)(distanceToTravel * PULSES_PER_CM);

  resetCount();
  movement = FORWARD; //TODO nowy typ TURN + remainingDegrees in loop

  //d.updateDashboard(degrees, currentSpeedL, currentSpeedR, movement);
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


int serwoWrite(int angle){
  serwoAngle = angle;
  serwo.write(angle);
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


// zmienia wartość pinu BEEPER
void doBeep() {
  digitalWrite(BEEPER, digitalRead(BEEPER) ^ 1);
}
