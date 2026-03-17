#include "Wheels.h"
#include "Display.h"


Wheels w;
Display d;
volatile char cmd;

unsigned long moveStart = 0;
unsigned long moveTime = 0;

volatile bool moving = false;

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
      case 's': w.stop(); break;
      case '1': w.setSpeedLeft(100); break;
      case '2': w.setSpeedLeft(200); break;
      case '9': w.setSpeedRight(100); break;
      case '0': w.setSpeedRight(200); break;
      case '5': w.setSpeed(200); break;
      case 'u': w.goForward(1); break;
      case 'j': w.goBack(1); break;
      case 'l': d.draw(10); break;
      case 'o': forwardNew(25); break;
      case 'p': backNew(25); break;
    }
  }
  if (moving) {

      unsigned long elapsed = millis() - moveStart;
      Serial.println(elapsed);

      if (elapsed >= moveTime) {
        w.stop();
        moving = false;
        d.draw(0);
      } 
      else {
        int remaining = totalDistance - (elapsed / 50);
        d.draw(remaining);
      }

    }
}


// void forwardNew(int cm) {
//   int time = cm*50;
//   w.forward();
//   delay(cm*50);
//   w.stop();
// }

void forwardNew(int cm) {
  moveTime = cm * 25;      // czas ruchu
  totalDistance = cm;

  moveStart = millis();
  moving = true;

  d.left();
  w.forward();
}

void backNew(int cm) {
  moveTime = cm * 25;      // czas ruchu
  totalDistance = cm;

  moveStart = millis();
  moving = true;

  w.back();
}