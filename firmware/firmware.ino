#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include "src/board/WaveshareSurface.h"
#include "src/board/BoardInputs.h"
#include "src/behavior/WearableBehavior.h"
#include "src/behavior/HeadingHistory.h"
#include "src/face/SettingsBatteryRenderer.h"
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
bool displayReady = false;
bool settingsVisible = false;
bool showRaw = false;
bool showJumpDebug = false;
bool scanned = false;
uint32_t lastFrameMs = 0;
uint32_t lastLogMs = 0;
char deviceId[13];
RTC_DATA_ATTR bool waitingForSleepRelease = false;
constexpr uint32_t kFramePeriodMs = 33;
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

void renderDirection(const nina::WearableOutput& output, bool headingValid) {
  surface.fillRect(0, 0, 240, 240, {0});
  Paint_DrawString_EN(79, 35, "PEER TEST", &Font12, BLACK, WHITE);
  Paint_DrawString_EN(102, 60, "FRONT", &Font12, BLACK, WHITE);
  Paint_DrawString_EN(75, 195,
      nina::calibrationStatusLabel(compass.calibrationAvailability()),
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

void enterDeepSleep(bool waitForRelease) {
  digitalWrite(2, LOW);
  pinMode(2, OUTPUT);
  gpio_hold_en(GPIO_NUM_2);
  gpio_deep_sleep_hold_en();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  waitingForSleepRelease = waitForRelease;
  if (waitForRelease) {
    // The IRQ can pulse during a hold, so check release before arming touch wake.
    esp_sleep_enable_timer_wakeup(250000);
  } else {
    rtc_gpio_pullup_en(GPIO_NUM_5);
    rtc_gpio_pulldown_dis(GPIO_NUM_5);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
  }
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(GPIO_NUM_2);
  rtc_gpio_deinit(GPIO_NUM_5);
  pinMode(5, INPUT_PULLUP);
  const auto wakeCause = esp_sleep_get_wakeup_cause();
  if (wakeCause == ESP_SLEEP_WAKEUP_TIMER && waitingForSleepRelease) {
    Wire.begin(6, 7);
    enterDeepSleep(nina::touchIsDown() || digitalRead(5) == LOW);
  }
  waitingForSleepRelease = false;
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    Wire.begin(6, 7);
    const uint32_t started = millis();
    while (millis() - started < nina::kSleepHoldMs) {
      if (!nina::touchIsDown()) enterDeepSleep(true);
      delay(20);
    }
    behavior.ignoreTouchUntilRelease();
    digitalWrite(13, HIGH);
    pinMode(13, OUTPUT);
  }
  if (!surface.begin()) {
    Serial.println("Display initialization failed");
    return;
  }
  displayReady = true;
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
}

void loop() {
  serialCommands();
  const uint32_t now = millis();
  if (now - lastFrameMs < kFramePeriodMs) { delay(1); return; }
  lastFrameMs = now;
  const auto motion = motionSampler.update(now);
  const auto& gravity = motion.gravity;
  const auto& acceleration = motion.acceleration;
  const auto& gyro = motion.gyro;
  const auto timestamp = motion.timestamp;
  const auto fresh = motion.fresh;
  const bool sampled = compass.update(now, gravity);
  const auto sampledHeading = compass.heading(now);
  headingHistory.record(now, sampledHeading.valid, sampledHeading.degrees);
  static bool sampledSinceLog = false;
  sampledSinceLog = sampledSinceLog || sampled;
  if (showRaw && now - lastLogMs >= 250) {
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
  nina::WearableInput input(now, nina::touchIsDown(), expression,
      nina::BatterySnapshot(nina::batteryMillivolts(now), false, false, 0, deviceId,
                            true, false, "prototype-13-14"), localIdentity);
  input.motionValid = motion.motionValid;
  input.accelerationValid = fresh;
  input.accelerationMagnitudeG = sqrtf(acceleration[0] * acceleration[0] +
      acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 1000.0f;
  input.rollDegrees = motion.rollDegrees;
  input.verticalAccelerationG = motion.verticalAccelerationG;
  nina::DiscoveryObservation observations[8];
  size_t count = 0;
  while (count < 8 && discovery.receive(observations[count])) ++count;
  nina::DirectionObservation directions[8];
  size_t directionCount = 0;
  if (radioReady) {
    radio.update(millis());
    nina::RadioPacket packet;
    while (directionCount < 8 && radio.receive(packet)) {
      if (headingHistory.associate(packet, millis(), directions[directionCount])) ++directionCount;
    }
  }
  input.elapsedMs = millis();
  const auto heading = compass.heading(input.elapsedMs);
  input.headingValid = heading.valid;
  input.headingDegrees = heading.degrees;
  auto output = behavior.update(input, observations, count, directions, directionCount);
  if (output.sleepRequested) enterDeepSleep(true);
  if (output.screen == nina::ScreenName::SettingsBattery && !settingsVisible)
    output.battery.millivolts = nina::batteryMillivolts(input.elapsedMs, true);
  settingsVisible = output.screen == nina::ScreenName::SettingsBattery;
  if (output.peerCount && !radioReady &&
      (lastRadioAttemptMs == 0 || input.elapsedMs - lastRadioAttemptMs >= 3000)) {
    lastRadioAttemptMs = input.elapsedMs ? input.elapsedMs : 1;
    radioReady = radio.begin(localIdentity);
    if (!radioReady) Serial.println("ESP-NOW initialization failed; will retry");
  } else if (!output.peerCount) {
    if (radioReady) radio.end();
    radioReady = false;
    lastRadioAttemptMs = 0;
  }
  if (showJumpDebug) {
    Serial.printf("[DEBUG-jump] %lu %lu %lu %d %d %.3f %.1f %.1f %.1f %.1f %.1f %.1f %d\n",
                  (unsigned long)now, (unsigned long)input.elapsedMs, (unsigned long)timestamp,
                  fresh, input.motionValid, input.verticalAccelerationG,
                  acceleration[0], acceleration[1], acceleration[2], gyro[0], gyro[1], gyro[2],
                  output.jumpOverlayActive);
  }
  if (!displayReady) return;
  static nina::SettingsBatteryRefresh settingsRefresh;
  if (output.screen == nina::ScreenName::PeerDirection) {
    settingsRefresh.invalidate();
    renderDirection(output, input.headingValid);
  } else if (output.screen == nina::ScreenName::Face) {
    settingsRefresh.invalidate();
    renderer.render(surface, output.face, 240, 240);
  } else {
    const auto lines = nina::settingsBatteryLines(output.battery);
    if (!settingsRefresh.changed(lines)) return;
    renderSettings(lines);
  }
  surface.present();
}
