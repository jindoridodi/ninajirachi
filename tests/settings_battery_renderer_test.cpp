#include "../firmware/src/face/SettingsBatteryRenderer.h"
#include <cassert>
#include <string>

struct Surface {
  std::string text;
  void render(const nina::BatterySnapshot& battery) {
    for (const auto& value : nina::settingsBatteryLines(battery).lines) {
      text += value;
      text += '\n';
    }
  }
};

int main() {
  Surface surface;
  nina::BatterySnapshot battery{3890, true, false, 72, "NINA-42", true, false, "v1"};
  surface.render(battery);
  assert(surface.text.find("3.890 V") != std::string::npos);
  assert(surface.text.find("Charge: Unknown") != std::string::npos);
  assert(surface.text.find("72%") == std::string::npos);
  assert(surface.text.find("NINA-42") != std::string::npos);
  assert(surface.text.find("v1") != std::string::npos);
  surface.text.clear();
  battery.hasEstimatedPercent = true;
  battery.chargingKnown = true;
  surface.render(battery);
  assert(surface.text.find("72%") != std::string::npos);
  assert(surface.text.find("Charge: Yes") != std::string::npos);
  surface.text.clear();
  battery.hasVoltage = false;
  surface.render(battery);
  assert(surface.text.find("Voltage: Unknown") != std::string::npos);
}
