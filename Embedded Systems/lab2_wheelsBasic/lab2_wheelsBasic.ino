#include "Wheels.h"
#include "Display.h"


Wheels w;
Display d;
volatile char cmd;

unsigned long moveStart = 0;
unsigned long moveTime = 0;

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
  // put your setup code here, to run once:
  w.attach(7,8,5,12,11,10);
  
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
      case 'x': w.back(); break;
      case 'a': w.forwardLeft(); break;
      case 'd': w.forwardRight(); break;
      case 'z': w.backLeft(); break;
      case 'c': w.backRight(); break;
      case 's': w.stop(); movement = NONE; d.updateDashboard(0, 0, 0, movement); break;
      case '1': currentSpeedL = 100; w.setSpeedLeft(100); break;
      case '2': currentSpeedL = 200; w.setSpeedLeft(200); break;
      case '9': currentSpeedR = 100; w.setSpeedRight(100); break;
      case '0': currentSpeedR = 200; w.setSpeedRight(200); break;
      case '5': currentSpeedL = 200; currentSpeedR = 200; w.setSpeed(200); break;
      case 'u': forwardNew(10); break;
      case 'j': backNew(10); break;
      case 'o': forwardNew(25); break;
      case 'p': backNew(25); break;
    }
  }
  if (movement != NONE) {
      unsigned long elapsed = millis() - moveStart;

      if (elapsed >= moveTime) {
        w.stop();
        movement = NONE;
        d.updateDashboard(0, 0, 0, movement);
      } 
      else {
        int remaining = totalDistance - (elapsed / 25);
        d.updateDashboard(remaining, currentSpeedL, currentSpeedR, movement);
      }

    }
}

void forwardNew(int cm) {
  moveTime = cm * 25;      // czas ruchu
  totalDistance = cm;

  moveStart = millis();
  movement = FORWARD;

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.forward();
}

void backNew(int cm) {
  moveTime = cm * 25;      // czas ruchu
  totalDistance = cm;

  moveStart = millis();
  movement = BACKWARD;

  d.updateDashboard(cm, currentSpeedL, currentSpeedR, movement);
  w.back();
}