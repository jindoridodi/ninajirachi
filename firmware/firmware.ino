#include "src/board/WaveshareSurface.h"
#include "src/board/BoardInputs.h"
#include "src/behavior/WearableBehavior.h"
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
uint64_t localIdentity = 0;
nina::CompassService compass;
nina::MotionSampler motionSampler(Wire);
TwoWire compassBus(1);
nina::ExpressionName expression = nina::ExpressionName::Idle;
bool displayReady = false;
bool compassReady = false;
bool showRaw = false;
bool showJumpDebug = false;
bool scanned = false;
uint32_t lastFrameMs = 0;
uint32_t lastLogMs = 0;
char deviceId[13];
constexpr uint32_t kFramePeriodMs = 33;
constexpr uint8_t kCompassAddress = 0x2C;
constexpr uint8_t kCompassSdaPin = 15;
constexpr uint8_t kCompassSclPin = 16;

bool compassAddressPresent() {
  compassBus.beginTransmission(kCompassAddress);
  return compassBus.endTransmission() == 0;
}

void restoreCompass() {
  Preferences prefs;
  if (!prefs.begin("nina-verified", true)) return;
  const bool verified = prefs.getUChar("address", 0) == kCompassAddress;
  prefs.end();
  if (verified && compassAddressPresent())
    compassReady = compass.begin(compassBus, nina::CompassConfig(true, kCompassAddress));
}

bool clearCompassCalibration() {
  Preferences prefs;
  if (!prefs.begin("nina-compass", false)) return false;
  const bool ok = !prefs.isKey("cal-v1") || prefs.remove("cal-v1");
  prefs.end();
  return ok;
}

void renderSettings(const nina::SettingsBatteryDisplay& settings) {
  const auto text = nina::settingsBatteryLines(settings);
  surface.fillRect(0, 0, 240, 240, {0});
  for (int line = 0; line < 7; ++line)
    // The Waveshare string wrapper passes these two colors to Paint_DrawChar in reverse.
    Paint_DrawString_EN(32, 50 + line * 20, text.lines[line], &Font12, BLACK, WHITE);
}

void serialCommands() {
  if (!Serial.available()) return;
  const char command = Serial.read();
  if (command == 's') {
    Serial.println("Scanning compass I2C bus on SDA15/SCL16...");
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
    Preferences verification;
    const bool opened = verification.begin("nina-verified", false);
    const bool forgotten = opened && verification.isKey("address") &&
                           verification.getUChar("address", 0) == 0;
    if (opened) verification.end();
    const bool safeCalibration = scanned && opened && (!forgotten || clearCompassCalibration());
    compassReady = safeCalibration && compass.begin(compassBus, nina::CompassConfig(true, kCompassAddress));
    Serial.println(compassReady ? "Compass enabled" : "Scan/verify QMC5883P first");
    if (compassReady) {
      Preferences prefs;
      if (!prefs.begin("nina-verified", false)) {
        Serial.println("Could not save compass verification; scan and verify again");
        compassReady = false;
      }
      else {
        if (prefs.putUChar("address", kCompassAddress) != 1) {
          Serial.println("Could not save compass verification; scan and verify again");
          compassReady = false;
        }
        prefs.end();
      }
      if (!compassReady) compass.begin(compassBus);
    }
  } else if (command == 'u') {
    Preferences prefs;
    bool forgotten = false;
    if (prefs.begin("nina-verified", false)) {
      forgotten = prefs.putUChar("address", 0) == 1;
      if (!forgotten) Serial.println("Could not clear compass verification; do not replace sensor");
      prefs.end();
    } else Serial.println("Could not clear compass verification");
    if (forgotten && !clearCompassCalibration()) Serial.println("Could not clear compass calibration; retry verification");
    compassReady = false;
    scanned = false;
    compass.begin(compassBus);
  } else if (command == 'c') {
    Serial.println(compass.startCalibration() ? "Rotate through all axes; w saves" : "Enable compass first");
  } else if (command == 'w') {
    Serial.println(compass.saveCalibration() ? "Calibration saved" : "Not enough coverage/samples or storage failed");
  } else if (command == 'x') {
    compass.cancelCalibration();
  } else if (command == 'r') {
    showRaw = !showRaw;
  } else if (command == 't') {
    behavior.triggerJump(millis());
    Serial.println("Jump reaction triggered");
  } else if (command == 'j') {
    showJumpDebug = !showJumpDebug;
    Serial.println(showJumpDebug ? "[DEBUG-jump] enabled" : "[DEBUG-jump] disabled");
  } else if (command >= '0' && command <= '2') {
    expression = static_cast<nina::ExpressionName>(command - '0');
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
  compassBus.begin(kCompassSdaPin, kCompassSclPin, 400000);
  nina::resetTouch();
  analogReadResolution(12);
  analogSetPinAttenuation(1, ADC_11db);
  motionSampler.begin();
  restoreCompass();
  const uint64_t identity = ESP.getEfuseMac();
  localIdentity = identity & 0xffffffffffffULL;
  if (!discovery.begin(localIdentity)) Serial.println("BLE discovery initialization failed");
  radioReady = radio.begin(localIdentity);
  if (!radioReady) Serial.println("ESP-NOW initialization failed");
  snprintf(deviceId, sizeof(deviceId), "%04X%08X", unsigned(identity >> 32) & 0xffff,
           unsigned(identity));
  Serial.println("s scan; v verify/enable compass; u forget verification; c calibrate; w save; x cancel; r raw; t test jump; j jump debug; 0 idle; 1 curious; 2 excited");
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
  const bool sampled = compassReady && compass.update(now, gravity);
  if (sampled && showRaw && now - lastLogMs >= 250) {
    lastLogMs = now;
    const auto raw = compass.raw();
    const auto heading = compass.heading(now);
    Serial.printf("mag %d %d %d heading %s %.1f samples %u\n", raw.x, raw.y, raw.z,
                  heading.valid ? "valid" : "unavailable", heading.degrees,
                  unsigned(compass.calibrationSamples()));
  }
  nina::WearableInput input(now, nina::touchIsDown(), expression,
      nina::BatterySnapshot(nina::batteryMillivolts(), false, false, 0, deviceId,
                            true, false, "prototype-13-14"), localIdentity);
  input.motionValid = motion.motionValid;
  input.accelerationValid = fresh;
  input.accelerationMagnitudeG = sqrtf(acceleration[0] * acceleration[0] +
      acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 1000.0f;
  input.rollDegrees = motion.rollDegrees;
  input.verticalAccelerationG = motion.verticalAccelerationG;
  nina::BlePacket packets[8];
  nina::DiscoveryObservation observations[8];
  size_t count = 0;
  while (count < 8 && discovery.receive(packets[count])) {
    const auto& packet = packets[count];
    observations[count++] = {packet.data, sizeof(packet.data), packet.rssi, packet.receivedMs};
  }
  nina::DirectionObservation directions[8];
  size_t directionCount = 0;
  if (radioReady) {
    radio.update(millis());
    nina::RadioPacket packet;
    while (directionCount < 8 && radio.receive(packet)) {
      const uint32_t observedMs = millis();
      if (observedMs - packet.receivedMs > 250) continue;
      const auto heading = compass.heading(observedMs);
      if (heading.valid)
        directions[directionCount++] = {packet.peer, packet.rssi, heading.degrees, observedMs};
    }
  }
  input.elapsedMs = millis();
  const auto heading = compass.heading(input.elapsedMs);
  input.headingValid = heading.valid;
  input.headingDegrees = heading.degrees;
  const auto output = behavior.update(input, observations, count, directions, directionCount);
  if (showJumpDebug) {
    Serial.printf("[DEBUG-jump] %lu %lu %lu %d %d %.3f %.1f %.1f %.1f %.1f %.1f %.1f %d\n",
                  (unsigned long)now, (unsigned long)input.elapsedMs, (unsigned long)timestamp,
                  fresh, input.motionValid, input.verticalAccelerationG,
                  acceleration[0], acceleration[1], acceleration[2], gyro[0], gyro[1], gyro[2],
                  output.jumpOverlayActive);
  }
  if (!displayReady) return;
  if (output.screen == nina::ScreenName::Face) renderer.render(surface, output.face, 240, 240);
  else renderSettings(output.settings);
  surface.present();
}
