#include <cstdint>
#include <cstddef>
#define _DEV_CONFIG_H_
#include "../firmware/src/board/MotionSampler.h"
#include <cassert>
#include <cmath>

unsigned char QMI8658_init() { return 1; }

int main() {
  TwoWire missing;
  nina::MotionSampler absent(missing);
  assert(!absent.begin() && !absent.update(0).fresh);
  TwoWire bus;
  bus.registers[0] = 5;
  bus.registers[53 + 5] = 0x10; // 4096 counts = 1 g on Z.
  nina::MotionSampler sampler(bus);
  assert(sampler.begin());
  bus.registers[48] = 1;
  auto sample = sampler.update(0);
  assert(sample.fresh && !sample.motionValid && sample.acceleration[2] == 1000);
  bus.registers[48] = 2;
  sample = sampler.update(250);
  assert(sample.fresh && sample.motionValid && sample.gravity.z == 1);
  sample = sampler.update(300);
  assert(!sample.fresh && !sample.motionValid && sample.gravity.z == 1);
  assert(sampler.update(351).gravity.z == 0);
  bus.nack = true;
  assert(!sampler.update(600).fresh && sampler.update(600).gravity.z == 0);
  bus.nack = false;
  bus.registers[48] = 3;
  sample = sampler.update(601);
  assert(sample.fresh && !sample.motionValid);
  bus.registers[48] = 4;
  sample = sampler.update(851);
  assert(sample.motionValid && std::fabs(sample.verticalAccelerationG - 1) < 0.001f);
  bus.registers[58] = 0x20; // Dynamic 2 g keeps movement gravity, not compass gravity.
  for (unsigned i = 1; i <= 10; ++i) {
    bus.registers[48] = 4 + i;
    sample = sampler.update(851 + i * 200);
    assert(sample.fresh && sample.gravity.z == 0);
    assert(sampler.update(852 + i * 200).gravity.z == 0);
    assert(sample.motionValid == (i < 10));
  }
  TwoWire wrapBus;
  wrapBus.registers[0] = 5;
  wrapBus.registers[58] = 0x10;
  nina::MotionSampler wrapped(wrapBus);
  assert(wrapped.begin());
  wrapBus.registers[48] = 1;
  wrapped.update(UINT32_MAX - 100);
  wrapBus.registers[48] = 2;
  assert(wrapped.update(149).motionValid);
}
