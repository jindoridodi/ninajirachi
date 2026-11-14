#include <Wire.h>
#include <cassert>

TwoWire Wire;
constexpr int OUTPUT = 1, LOW = 0, HIGH = 1;
void pinMode(int, int) {}
void digitalWrite(int, int) {}
void delay(int) {}
uint32_t adcMillivolts = 1300;
uint32_t analogReadMilliVolts(int pin) { assert(pin == 1); return adcMillivolts; }

#include "../firmware/src/board/BoardInputs.h"

int main() {
  Wire.registers[2] = 1; assert(nina::touchReading() == 1);
  Wire.nack = true; assert(nina::touchReading() == -1);
  Wire.nack = false; Wire.badRead = true; assert(nina::touchReading() == -1);
  Wire.badRead = false; Wire.registers[2] = 0; assert(nina::touchReading() == 0);
  assert(nina::batteryMillivolts(0) == 3900);
  adcMillivolts = 1200;
  assert(nina::batteryMillivolts(999) == 3900);
  assert(nina::batteryMillivolts(1000) == 3600);
  adcMillivolts = 1250;
  assert(nina::batteryMillivolts(1001, true) == 3750);
  adcMillivolts = 1100;
  assert(nina::batteryMillivolts(2000) == 3750);
  assert(nina::batteryMillivolts(2001) == 3300);
  nina::batteryMillivolts(UINT32_MAX - 100, true);
  adcMillivolts = 1000;
  assert(nina::batteryMillivolts(898) == 3300);
  assert(nina::batteryMillivolts(899) == 3000);
}
