#include "src/board/WaveshareSurface.h"
#include "src/board/BoardInputs.h"
#include "src/behavior/WearableBehavior.h"
#include "src/face/SettingsBatteryRenderer.h"
#include "src/compass/CompassService.h"

namespace {

nina::WaveshareSurface surface;
nina::FaceRenderer renderer;
nina::WearableBehavior behavior;
nina::CompassService compass;
nina::SettingsBatteryRenderer settingsRenderer;
nina::ExpressionName expression = nina::ExpressionName::Idle;
bool displayReady = false;
bool imuReady = false;
bool compassReady = false;
bool showRaw = false;
bool scanned = false;
uint32_t lastFrameMs = 0;
uint32_t lastLogMs = 0;
char deviceId[13];
constexpr uint32_t kFramePeriodMs = 33;
constexpr uint8_t kCompassAddress = 0x2C;

class SettingsSurface : public nina::SettingsDrawSurface {
 public:
  void fillRect(int x, int y, int w, int h, nina::Color color) override {
    surface.fillRect(x, y, w, h, color);
  }
  void drawText(int x, int y, const char* text, nina::Color color) override {
    uint16_t rgb = ((color.rgb >> 8) & 0xf800) | ((color.rgb >> 5) & 0x07e0) |
                   ((color.rgb >> 3) & 0x001f);
    Paint_DrawString_EN(x, y, text, &Font12, rgb, BLACK);
  }
} settingsSurface;

void serialCommands() {
  if (!Serial.available()) return;
  const char command = Serial.read();
  if (command == 's') {
    scanned = false;
    for (uint8_t address = 1; address < 127; ++address) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) {
        Serial.printf("I2C 0x%02X\n", address);
        if (address == kCompassAddress) scanned = true;
      }
    }
  } else if (command == 'v') {
    // User confirms physical QMC5883P marking, wiring, and aligned IMU/compass axes.
    compassReady = scanned && compass.begin(Wire, nina::CompassConfig(true, kCompassAddress));
    Serial.println(compassReady ? "Compass enabled" : "Scan/verify QMC5883P first");
  } else if (command == 'c') {
    Serial.println(compass.startCalibration() ? "Rotate through all axes; w saves" : "Enable compass first");
  } else if (command == 'w') {
    Serial.println(compass.saveCalibration() ? "Calibration saved" : "Not enough coverage/samples or storage failed");
  } else if (command == 'x') {
    compass.cancelCalibration();
  } else if (command == 'r') {
    showRaw = !showRaw;
  } else if (command >= '0' && command <= '4') {
    const nina::ExpressionName names[] = {nina::ExpressionName::Idle, nina::ExpressionName::Curious,
      nina::ExpressionName::Happy, nina::ExpressionName::Surprised, nina::ExpressionName::Excited};
    expression = names[command - '0'];
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!surface.begin()) {
    Serial.println("Display initialization failed");
    return;
  }
  displayReady = true;
  nina::resetTouch();
  analogReadResolution(12);
  analogSetPinAttenuation(1, ADC_11db);
  imuReady = QMI8658_init() != 0;
  const uint64_t identity = ESP.getEfuseMac();
  snprintf(deviceId, sizeof(deviceId), "%04X%08X", unsigned(identity >> 32) & 0xffff,
           unsigned(identity));
  Serial.println("s scan; v verify/enable compass; c calibrate; w save; x cancel; r raw; 0-4 expressions");
  renderer.render(surface, nina::idlePreset(), 240, 240);
  surface.present();
}

void loop() {
  serialCommands();
  const uint32_t now = millis();
  if (now - lastFrameMs < kFramePeriodMs) { delay(1); return; }
  lastFrameMs = now;
  nina::MagneticVector gravity{0, 0, 0};
  if (imuReady) {
    float acceleration[3], gyro[3];
    unsigned int stamp;
    QMI8658_read_xyz(acceleration, gyro, &stamp);
    // The unmodified official driver returns acceleration in mg.
    gravity = {acceleration[0] / 1000, acceleration[1] / 1000, acceleration[2] / 1000};
  }
  const bool sampled = compassReady && compass.update(now, gravity);
  if (sampled && showRaw && now - lastLogMs >= 250) {
    lastLogMs = now;
    const auto raw = compass.raw();
    const auto heading = compass.heading(now);
    Serial.printf("mag %d %d %d heading %s %.1f samples %u\n", raw.x, raw.y, raw.z,
                  heading.valid ? "valid" : "unavailable", heading.degrees,
                  unsigned(compass.calibrationSamples()));
  }
  const auto output = behavior.update({now, nina::touchIsDown(), expression,
      nina::BatterySnapshot(nina::batteryMillivolts(), false, false, 0, deviceId,
                            true, false, "prototype-13-14")});
  if (!displayReady) return;
  if (output.screen == nina::ScreenName::Face) renderer.render(surface, output.face, 240, 240);
  else settingsRenderer.render(settingsSurface, output.settings, 240, 240);
  surface.present();
}
