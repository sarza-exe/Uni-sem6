#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Display.h"


byte LCDAddress = 0x27;

LiquidCrystal_I2C lcd(LCDAddress, 16, 2);

uint8_t arrowRight[8] =
{
    0b01000,
    0b01100,
    0b00110,
    0b11111,
    0b11111,
    0b00110,
    0b01100,
    0b01000
};

int argIn = 100;

Display::Display() {
 }

void Display::init() {
  lcd.init();
  lcd.backlight();

  lcd.createChar(0, arrowRight);
}

void Display::draw(int value)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(value);
}

void Display::left()
{
    lcd.setCursor(1, 1);
    lcd.print('=');
    lcd.setCursor(1,0);
    lcd.print('<');
}

void Display::right()
{
    lcd.setCursor(1, 1);
    lcd.print('=');
}

void Display::stop()
{
    lcd.setCursor(1, 1);
    lcd.print('=');
}



