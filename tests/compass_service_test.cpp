#include "../firmware/src/compass/CompassService.h"
#include <cassert>
#include <cmath>

std::vector<uint8_t> Preferences::bytes;
bool Preferences::failOpen = false;
bool Preferences::failWrite = false;

int main() {
  TwoWire bus;
  bus.registers[0] = 0x80;
  nina::CompassService compass;
  assert(!compass.begin(bus));
  assert(bus.transactions == 0); // Disabled means no bus activity.
  assert(compass.begin(bus, {true, 0x2c}));
  assert(bus.registers[0x29] == 0x06);
  assert(bus.registers[0x0b] == 0x08);
  assert(bus.registers[0x0a] == 0xcd);
  bus.sample(100, 200, -300);
  assert(compass.update(100, {0, 0, 1}));
  assert(compass.raw().z == -300);
  assert(!compass.heading(100).valid); // No calibration yet.
  assert(compass.startCalibration());
  assert(!compass.saveCalibration());
  for (unsigned i = 0; i < 120; ++i) {
    bus.sample((i & 1) ? 1000 : -1000, (i & 2) ? 1000 : -1000,
               (i & 4) ? 1000 : -1000);
    assert(compass.update(110 + i, {0, 0, 1}));
  }
  assert(compass.saveCalibration());
  nina::CompassService restored;
  assert(restored.begin(bus, {true, 0x2c}));
  bus.sample(1000, 1000, 0);
  assert(restored.update(300, {0, 0, 1}));
  assert(restored.heading(300).valid);
  assert(std::fabs(restored.heading(300).degrees - 45) < 0.01f);
  assert(!restored.heading(551).valid);
  bus.registers[9] = 3;
  assert(!restored.update(301, {0, 0, 1}));
  assert(!restored.heading(301).valid); // Never reuse a heading after overflow.
  bus.sample(0, 0, 0);
  assert(!restored.update(302, {0, 0, 1}));
  bus.sample(32767, 1, 1);
  assert(!restored.update(303, {0, 0, 1}));
  bus.sample(1000, 0, 0);
  bus.shortRead = true;
  assert(!restored.update(304, {0, 0, 1}));
  bus.shortRead = false;
  bus.badRead = true;
  assert(!restored.update(305, {0, 0, 1}));
  bus.badRead = false;
  bus.registers[9] = 0;
  assert(!restored.update(306, {0, 0, 1}));
  bus.registers[9] = 1;
  assert(restored.update(307, {0, 0, 2}));
  assert(!restored.heading(307).valid);
  assert(restored.update(UINT32_MAX - 10, {0, 0, 1}));
  assert(restored.heading(10).valid); // millis() wraparound.
  assert(!restored.heading(300).valid);

  assert(restored.startCalibration());
  for (unsigned i = 0; i < 120; ++i) {
    bus.sample((i & 1) ? 2000 : -2000, (i & 2) ? 1000 : -1000,
               (i & 4) ? 1000 : -1000);
    assert(restored.update(400 + i, {0, 0, 1}));
  }
  assert(!restored.heading(519).valid);
  Preferences::failWrite = true;
  assert(!restored.saveCalibration());
  Preferences::failWrite = false;
  assert(!restored.saveCalibration(NAN));
  restored.cancelCalibration();
  bus.sample(1000, 1000, 0);
  assert(restored.update(600, {0, 0, 1}));
  assert(std::fabs(restored.heading(600).degrees - 45) < 0.01f); // Old calibration survives failure/cancel.

  assert(restored.startCalibration());
  for (unsigned i = 0; i < 120; ++i) {
    bus.sample(i & 1 ? 1000 : -1000, 50, 50);
    restored.update(700 + i, {0, 0, 1});
  }
  assert(!restored.saveCalibration()); // Repeated/one-axis movement is insufficient.
  restored.cancelCalibration();

  nina::CompassService invalid;
  bus.registers[0] = 0;
  assert(!invalid.begin(bus, {true, 0x2c}));
  assert(!invalid.update(900, {0, 0, 1}));
  bus.registers[0] = 0x80;
  bus.registers[0x29] = 0;
  bus.ignoreWrites = true;
  assert(!invalid.begin(bus, {true, 0x2c}));
  bus.ignoreWrites = false;
  bus.nack = true;
  assert(!invalid.begin(bus, {true, 0x2c}));
  bus.nack = false;

  Preferences::bytes[0] ^= 1; // Unknown storage version is never accepted.
  assert(invalid.begin(bus, {true, 0x2c}));
  assert(!invalid.calibrated());
  assert(invalid.update(901, {0, 0, 1}));
  assert(!invalid.heading(901).valid);
  Preferences::bytes.resize(2);
  assert(invalid.begin(bus, {true, 0x2c}));
  assert(!invalid.calibrated());
  Preferences::failOpen = true;
  assert(invalid.begin(bus, {true, 0x2c})); // Raw inspection still works if NVS fails.
  assert(!invalid.calibrated());
  assert(!invalid.begin(bus));
  assert(!invalid.startCalibration());
}
