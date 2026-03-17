#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Display.h"

byte LCDAddress = 0x27;
LiquidCrystal_I2C lcd(LCDAddress, 16, 2);

// Definicje własnych znaków
uint8_t arrowRight[8] = {0b01000, 0b01100, 0b00110, 0b11111, 0b11111, 0b00110, 0b01100, 0b01000};
uint8_t arrowLeft[8]  = {0b00010, 0b00110, 0b01100, 0b11111, 0b11111, 0b01100, 0b00110, 0b00010};

Display::Display() {}

void Display::init() {
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, arrowRight);
    lcd.createChar(1, arrowLeft);
    lcd.clear();
}

void Display::updateDashboard(int remainingDist, int speedL, int speedR, int movement) {
    // line 1
    lcd.setCursor(0, 0);
    lcd.print("Dist: ");
    lcd.print(remainingDist);
    lcd.print(" cm    "); // Spacje czyszczą pozostałości po dłuższych liczbach

    // line 2
    // left wheels (Speed L)
    lcd.setCursor(0, 1);
    if (movement == 2) lcd.print("-"); // BACKWARD
    lcd.print(speedL);
    lcd.print("  "); // Czyści resztki

    // animation
    lcd.setCursor(6, 1);
    if (movement == 1) { // FORWARD
        // Prosta animacja migająca strzałkami zależna od czasu
        if ((millis() / 250) % 2 == 0) lcd.print(" >> ");
        else lcd.write(0); lcd.print("  "); lcd.write(0);
    } 
    else if (movement == 2) { // BACKWARD
        if ((millis() / 250) % 2 == 0) lcd.print(" << ");
        else lcd.write(1); lcd.print("  "); lcd.write(1);
    } 
    else { // STOP (NONE)
        lcd.setCursor(5, 1);
        lcd.print("[STOP]");
    }

    // right wheels (Speed R)
    lcd.setCursor(12, 1);
    if (movement == 2) lcd.print("-"); // BACKWARD
    lcd.print(speedR);
    lcd.print("  ");
}