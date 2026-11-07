#include "../firmware/src/face/SettingsBatteryRenderer.h"
#include <cassert>
#include <string>

struct Surface {
  std::string text;
  void render(const nina::SettingsBatteryDisplay& settings) {
    for (const auto& value : nina::settingsBatteryLines(settings).lines) {
      text += value;
      text += '\n';
    }
  }
};

int main() {
  Surface surface;
  nina::SettingsBatteryDisplay settings{{3890, true, false, 72, "NINA-42", true, false, "v1"}};
  surface.render(settings);
  assert(surface.text.find("3.890 V") != std::string::npos);
  assert(surface.text.find("Charge: Unknown") != std::string::npos);
  assert(surface.text.find("72%") == std::string::npos);
  assert(surface.text.find("NINA-42") != std::string::npos);
  assert(surface.text.find("v1") != std::string::npos);
  surface.text.clear();
  settings.battery.hasEstimatedPercent = true;
  settings.battery.chargingKnown = true;
  surface.render(settings);
  assert(surface.text.find("72%") != std::string::npos);
  assert(surface.text.find("Charge: Yes") != std::string::npos);
  surface.text.clear();
  settings.battery.hasVoltage = false;
  surface.render(settings);
  assert(surface.text.find("Voltage: Unknown") != std::string::npos);
}
