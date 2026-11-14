#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include "src/board/WaveshareSurface.h"
#include "src/board/BoardInputs.h"
#include "src/board/SleepRuntime.h"
#include "src/board/RuntimeSchedule.h"
#include "src/board/RuntimeInputs.h"
#include "src/board/Runtime.h"
#include "src/behavior/WearableBehavior.h"
#include "src/behavior/HeadingHistory.h"
#include "src/face/FramePresenter.h"
#include "src/compass/CompassService.h"
#include "src/board/MotionSampler.h"
#include "src/discovery/BleDiscovery.h"
#include "src/discovery/EspNowDiscovery.h"

namespace {

nina::WaveshareSurface surface;
nina::FaceRenderer renderer;
nina::WearableBehavior behavior;
nina::BleDiscovery discovery;
nina::EspNowDiscovery radio;
bool radioReady = false;
uint32_t lastRadioAttemptMs = 0;
uint64_t localIdentity = 0;
nina::CompassService compass;
nina::HeadingHistory headingHistory;
nina::MotionSampler motionSampler(Wire);
TwoWire compassBus(1);
nina::ExpressionName expression = nina::ExpressionName::Idle;
nina::Runtime runtime;
bool settingsVisible = false;
bool showRaw = false;
bool showJumpDebug = false;
bool scanned = false;
nina::RuntimeSchedule runtimeSchedule;
nina::WearableOutput latestOutput{};
bool latestHeadingValid = false;
uint32_t lastLogMs = 0;
char deviceId[13];
RTC_DATA_ATTR bool waitingForSleepRelease = false;
constexpr uint8_t kCompassAddress = 0x2C;
constexpr uint8_t kCompassSdaPin = 16;
constexpr uint8_t kCompassSclPin = 15;

void restoreCompass() {
  compass.restore(compassBus, kCompassAddress);
}

void renderSettings(const nina::SettingsBatteryLines& text) {
  surface.fillRect(0, 0, 240, 240, {0});
  for (int line = 0; line < 7; ++line)
    // The Waveshare string wrapper passes these two colors to Paint_DrawChar in reverse.
    Paint_DrawString_EN(32, 50 + line * 20, text.lines[line], &Font12, BLACK, WHITE);
}

void renderDirection(const nina::WearableOutput& output, bool headingValid, const char* calibrationLabel) {
  surface.fillRect(0, 0, 240, 240, {0});
  Paint_DrawString_EN(79, 35, "PEER TEST", &Font12, BLACK, WHITE);
  Paint_DrawString_EN(102, 60, "FRONT", &Font12, BLACK, WHITE);
  Paint_DrawString_EN(75, 195,
      calibrationLabel,
      &Font12, BLACK, WHITE);
  surface.fillRect(116, 116, 8, 8, {0xffffff});
  if (isfinite(output.relativeDirection)) {
    const float radians = output.relativeDirection * 3.14159265f / 180.0f;
    const int x = 120 + lroundf(65 * sinf(radians));
    const int y = 120 - lroundf(65 * cosf(radians));
    surface.fillRect(x - 6, y - 6, 12, 12, {0x00ff00});
  } else {
    const char* status = !output.targetId ? "NO PEER" : !headingValid ? "NO HEADING" : "TURN TO SAMPLE";
    Paint_DrawString_EN(65, 175, status, &Font12, BLACK, WHITE);
  }
}

void serialCommands() {
  if (!Serial.available()) return;
  const char command = Serial.read();
  if (command == 's') {
    Serial.println("Scanning compass I2C bus on SDA16/SCL15...");
    scanned = false;
    uint8_t found = 0;
    for (uint8_t address = 1; address < 127; ++address) {
      compassBus.beginTransmission(address);
      if (compassBus.endTransmission() == 0) {
        Serial.printf("I2C 0x%02X\n", address);
        ++found;
        if (address == kCompassAddress) scanned = true;
      }
    }
    if (found == 0) Serial.println("No I2C devices found");
    else if (!scanned) Serial.println("QMC5883P not found at 0x2C");
  } else if (command == 'v') {
    // User confirms physical QMC5883P marking, wiring, and aligned IMU/compass axes.
    compass.verify(compassBus, kCompassAddress, scanned);
    Serial.println(compass.ready() ? "Compass enabled" : "Scan/verify QMC5883P first or check storage");
  } else if (command == 'u') {
    bool cleared = false;
    if (!compass.forgetVerification(compassBus, cleared))
      Serial.println("Could not clear compass verification; do not replace sensor");
    else if (!cleared)
      Serial.println("Could not clear compass calibration; retry verification");
    scanned = false;
  } else if (command == 'c') {
    Serial.println(compass.startCalibration() ? "Rotate through all axes; w saves" : "Enable compass and check calibration storage first");
  } else if (command == 'w') {
    const bool saved = compass.saveCalibration();
    const auto data = compass.collectionData();
    Serial.printf("Calibration %s; samples %u; spans X=%d Y=%d Z=%d; status=%s\n", saved ? "saved" : "failed",
                  unsigned(compass.calibrationSamples()), int(data.maximum.x) - data.minimum.x,
                  int(data.maximum.y) - data.minimum.y, int(data.maximum.z) - data.minimum.z,
                  compass.saveStatus() == nina::CalibrationSaveStatus::Ok ? "ok" :
                  compass.saveStatus() == nina::CalibrationSaveStatus::NotReady ? "not collecting or insufficient samples" :
                  compass.saveStatus() == nina::CalibrationSaveStatus::Coverage ? "insufficient coverage or invalid offset" : "storage write failed");
  } else if (command == 'x') {
    compass.cancelCalibration();
  } else if (command == 'r') {
    showRaw = !showRaw;
  } else if (command == 'd') {
    behavior.toggleDirection(millis());
    Serial.println("Peer direction display toggled");
  } else if (command == 't') {
    behavior.triggerJump(millis());
    Serial.println("Jump reaction triggered");
  } else if (command == 'j') {
    showJumpDebug = !showJumpDebug;
    Serial.println(showJumpDebug ? "[DEBUG-jump] enabled" : "[DEBUG-jump] disabled");
  } else if (command == 'b') {
    behavior.showSettings(millis());
    Serial.println("Settings screen enabled");
  } else if (command >= '0' && command <= '2') {
    expression = static_cast<nina::ExpressionName>(command - '0');
  }
}

}  // namespace

struct SleepBoard {
  void prepareWake() {
    gpio_deep_sleep_hold_dis(); gpio_hold_dis(GPIO_NUM_2);
    rtc_gpio_deinit(GPIO_NUM_5); pinMode(5, INPUT_PULLUP);
  }
  int wakeCause() {
    const auto cause = esp_sleep_get_wakeup_cause();
    return cause == ESP_SLEEP_WAKEUP_TIMER ? 1 : cause == ESP_SLEEP_WAKEUP_EXT0 ? 2 : 0;
  }
  void beginTouch() { Wire.begin(6, 7); }
  int touchReading() { return nina::touchReading(); }
  bool touchIrqLow() { return digitalRead(5) == LOW; }
  void darken() { digitalWrite(2, LOW); pinMode(2, OUTPUT); }
  void holdBacklight() { gpio_hold_en(GPIO_NUM_2); gpio_deep_sleep_hold_en(); }
  void disableWake() { esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); }
  void timerWake() { esp_sleep_enable_timer_wakeup(250000); }
  void touchWake() {
    rtc_gpio_pullup_en(GPIO_NUM_5); rtc_gpio_pulldown_dis(GPIO_NUM_5);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
  }
  void deepSleep() { esp_deep_sleep_start(); }
  uint32_t now() { return millis(); }
  void wait() { delay(20); }
  void suppressTouch() { behavior.ignoreTouchUntilRelease(); }
  void restoreTouchPower() { digitalWrite(13, HIGH); pinMode(13, OUTPUT); }
};
SleepBoard sleepBoard;

void enterDeepSleep(bool waitForRelease) {
  nina::enterSleep(sleepBoard, waitingForSleepRelease, waitForRelease);
}
void setup() {
  Serial.begin(115200);
  const auto wakeCause = esp_sleep_get_wakeup_cause();
  if (!nina::wakeRuntime(sleepBoard, waitingForSleepRelease, nina::kSleepHoldMs)) return;
  if (!runtime.start(surface, [&]() {
    compassBus.begin(kCompassSdaPin, kCompassSclPin, 400000);
    if (wakeCause != ESP_SLEEP_WAKEUP_EXT0) nina::resetTouch();
    analogReadResolution(12);
    analogSetPinAttenuation(1, ADC_11db);
    if (!motionSampler.begin()) Serial.println("QMI8658 motion initialization failed");
    restoreCompass();
    const uint64_t identity = ESP.getEfuseMac();
    localIdentity = identity & 0xffffffffffffULL;
    if (!discovery.begin(localIdentity)) Serial.println("BLE discovery initialization failed");
    snprintf(deviceId, sizeof(deviceId), "%04X%08X", unsigned(identity >> 32) & 0xffff,
             unsigned(identity));
    Serial.println("s scan; v verify/enable compass; u forget verification; c calibrate; w save; x cancel; r raw; d peer direction; t test jump; j jump debug; b settings; 0 idle; 1 curious; 2 excited");
    renderer.render(surface, nina::idlePreset(), 240, 240);
    surface.present();
  })) Serial.println("Display initialization failed");
}

struct InputBoard {
  bool touchDown = false;
  bool touchReadFailed = false;
  uint32_t lastTouchReadMs = 0;
  nina::MotionSampler& motionSampler = ::motionSampler;
  nina::CompassService& compass = ::compass;
  nina::HeadingHistory& headingHistory = ::headingHistory;
  nina::BleDiscovery& discovery = ::discovery;
  nina::EspNowDiscovery& radio = ::radio;
  nina::WearableBehavior& behavior = ::behavior;
  nina::ExpressionName& expression = ::expression;
  uint64_t& localIdentity = ::localIdentity;
  uint32_t& lastRadioAttemptMs = ::lastRadioAttemptMs;
  bool& radioReady = ::radioReady;
  bool& settingsVisible = ::settingsVisible;
  nina::WearableOutput& latestOutput = ::latestOutput;
  bool& latestHeadingValid = ::latestHeadingValid;
  uint32_t now() { return millis(); }
  int touch() { return nina::touchReading(); }
  nina::BatterySnapshot battery(uint32_t time) {
    return {nina::batteryMillivolts(time), false, false, 0, deviceId, true, false, "prototype-13-14"};
  }
  uint16_t refreshBattery(uint32_t time) { return nina::batteryMillivolts(time, true); }
  void sleep() { enterDeepSleep(true); }
  void radioFailure() { Serial.println("ESP-NOW initialization failed; will retry"); }
  void logCompass(uint32_t now, bool sampled) {
    static bool sampledSinceLog = false;
    sampledSinceLog = sampledSinceLog || sampled;
    if (!showRaw || now - lastLogMs < 250) return;
    lastLogMs = now;
    if (!sampledSinceLog) {
      const auto status = compass.readStatus();
      Serial.printf("mag unavailable: %s\n", status == nina::CompassReadStatus::Disabled ? "disabled" :
                    status == nina::CompassReadStatus::Communication ? "I2C error" :
                    status == nina::CompassReadStatus::Overflow ? "overflow" :
                    status == nina::CompassReadStatus::NoData ? "no new data" : "invalid sample");
    } else {
      const auto raw = compass.raw();
      const auto heading = compass.heading(now);
      Serial.printf("mag %d %d %d heading %s %.1f samples %u\n", raw.x, raw.y, raw.z,
                    heading.valid ? "valid" : "unavailable", heading.degrees,
                    unsigned(compass.calibrationSamples()));
    }
    sampledSinceLog = false;
  }
  void logMovement(uint32_t now, const nina::WearableInput& input, const nina::MotionSample& motion,
                   const nina::WearableOutput& output) {
    if (!showJumpDebug) return;
    Serial.printf("[DEBUG-jump] %lu %lu %lu %d %d %.3f %.1f %.1f %.1f %.1f %.1f %.1f %d\n",
                  (unsigned long)now, (unsigned long)input.elapsedMs, (unsigned long)motion.timestamp,
                  motion.fresh, input.motionValid, input.verticalAccelerationG,
                  motion.acceleration[0], motion.acceleration[1], motion.acceleration[2],
                  motion.gyro[0], motion.gyro[1], motion.gyro[2], output.jumpOverlayActive);
  }
};
struct DisplayBoard {
  void draw(const nina::WearableOutput& output, bool headingValid, const char* calibrationLabel) {
    if (output.screen == nina::ScreenName::PeerDirection)
      renderDirection(output, headingValid, calibrationLabel);
    else if (output.screen == nina::ScreenName::Face)
      renderer.render(surface, output.face, 240, 240);
    else
      renderSettings(nina::settingsBatteryLines(output.battery));
  }
  void present() { surface.present(); }
};

void renderFrame() {
  static nina::FramePresenter presenter;
  static DisplayBoard display;
  presenter.present(display, latestOutput, latestHeadingValid,
                    nina::calibrationStatusLabel(compass.calibrationAvailability()));
}

struct RuntimeBoard {
  uint32_t now() const { return millis(); }
  void update(uint32_t time) { static InputBoard inputs; nina::updateRuntimeInputs(inputs, time); }
  void render() { renderFrame(); }
};

void loop() {
  runtime.update([]() {
    serialCommands();
    discovery.update(millis());
    static RuntimeBoard board;
    runtimeSchedule.run(board);
    delay(1);
  }, []() { delay(100); });
}
