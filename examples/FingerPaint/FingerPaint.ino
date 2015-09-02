//#include <Wire.h>
//#include <SPI.h>

#include <Digole.h>

Digole::DigoleSerial LCD(Serial, 115200);

void setup() {
  LCD.begin();
  LCD.clearScreen();
  LCD.setRotation(Digole::ROT270);
  LCD.setColor(0xff, 0xff, 0xff);
}

void loop() {
  uint16_t x, y;

  LCD.readTouchscreen(x, y, Digole::TOUCH_DOWN_NONBLOCKING);
  if (x < 0xf000 && y < 0xf000) {
    LCD.drawPixel(x, y);
  }
}
