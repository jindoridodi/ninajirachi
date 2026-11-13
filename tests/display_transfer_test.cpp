#include "../firmware/src/waveshare/LCD_1in28.h"
#include <algorithm>
#include <cassert>
#include <vector>

SerialType Serial;
TwoWire Wire;
std::vector<uint8_t> sent;
std::vector<uint32_t> writes;
extern SPIClass* vspi;

int main() {
  SPIClass spi;
  vspi = &spi;
  UWORD frame[LCD_1IN28_WIDTH * LCD_1IN28_HEIGHT];
  auto* bytes = reinterpret_cast<uint8_t*>(frame);
  for (unsigned i = 0; i < sizeof(frame); ++i) bytes[i] = uint8_t(i * 17 + i / 480);
  const std::vector<uint8_t> original(bytes, bytes + sizeof(frame));

  LCD_1IN28_Display(frame);

  assert(writes.size() == 240);
  for (auto size : writes) assert(size == 480);
  assert(sent == original);
  assert(std::equal(original.begin(), original.end(), bytes));
}
UWORD* BlackImage = nullptr;
