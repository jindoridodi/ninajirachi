#include "src/board/WaveshareSurface.h"
#include "src/board/BoardInputs.h"
#include "src/behavior/WearableBehavior.h"
#include "src/face/SettingsBatteryRenderer.h"
#include "src/compass/CompassService.h"
#include "src/compass/GravityEstimator.h"
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
nina::GravityEstimator gravityEstimator;
nina::MagneticVector movementGravity{0, 0, 0};
uint32_t movementGravityAtMs = 0;
bool haveMovementGravity = false;
nina::ExpressionName expression = nina::ExpressionName::Idle;
bool displayReady = false;
bool imuReady = false;
uint8_t imuAddress = 0;
uint32_t lastImuTimestamp = 0;
bool haveImuTimestamp = false;
uint32_t lastImuSuccessAtMs = 0;
bool compassReady = false;
bool showRaw = false;
bool scanned = false;
uint32_t lastFrameMs = 0;
uint32_t lastLogMs = 0;
char deviceId[13];
constexpr uint32_t kFramePeriodMs = 33;
constexpr uint8_t kCompassAddress = 0x2C;

bool compassAddressPresent() {
  Wire.beginTransmission(kCompassAddress);
  return Wire.endTransmission() == 0;
}

bool readImuRegisters(uint8_t address, uint8_t reg, uint8_t* data, size_t size) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address, size) != size) return false;
  for (size_t i = 0; i < size; ++i) {
    const int value = Wire.read();
    if (value < 0) return false;
    data[i] = static_cast<uint8_t>(value);
  }
  return true;
}

uint8_t findImu() {
  for (uint8_t address = 0x6a; address <= 0x6b; ++address) {
    uint8_t id = 0;
    if (readImuRegisters(address, QMI8658Register_WhoAmI, &id, 1) && id == 0x05) return address;
  }
  return 0;
}

void restoreCompass() {
  Preferences prefs;
  if (!prefs.begin("nina-verified", true)) return;
  const bool verified = prefs.getUChar("address", 0) == kCompassAddress;
  prefs.end();
  if (verified && compassAddressPresent())
    compassReady = compass.begin(Wire, nina::CompassConfig(true, kCompassAddress));
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
    Paint_DrawString_EN(32, 50 + line * 20, text.lines[line], &Font12, WHITE, BLACK);
}

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
    Preferences verification;
    const bool opened = verification.begin("nina-verified", false);
    const bool forgotten = opened && verification.isKey("address") &&
                           verification.getUChar("address", 0) == 0;
    if (opened) verification.end();
    const bool safeCalibration = scanned && opened && (!forgotten || clearCompassCalibration());
    compassReady = safeCalibration && compass.begin(Wire, nina::CompassConfig(true, kCompassAddress));
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
      if (!compassReady) compass.begin(Wire);
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
    compass.begin(Wire);
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
  if (imuReady) { imuAddress = findImu(); imuReady = imuAddress != 0; }
  restoreCompass();
  const uint64_t identity = ESP.getEfuseMac();
  localIdentity = identity & 0xffffffffffffULL;
  if (!discovery.begin(localIdentity)) Serial.println("BLE discovery initialization failed");
  radioReady = radio.begin(localIdentity);
  if (!radioReady) Serial.println("ESP-NOW initialization failed");
  snprintf(deviceId, sizeof(deviceId), "%04X%08X", unsigned(identity >> 32) & 0xffff,
           unsigned(identity));
  Serial.println("s scan; v verify/enable compass; u forget verification; c calibrate; w save; x cancel; r raw; 0-4 expressions");
  renderer.render(surface, nina::idlePreset(), 240, 240);
  surface.present();
}

void loop() {
  serialCommands();
  const uint32_t now = millis();
  if (now - lastFrameMs < kFramePeriodMs) { delay(1); return; }
  lastFrameMs = now;
  nina::MagneticVector gravity{0, 0, 0};
  float rollDegrees = 0, accelerationG = 0;
  bool motionValid = false;
  if (imuReady) {
    float acceleration[3] = {}, gyro[3] = {};
    uint8_t stamp[3] = {}, raw[12] = {};
    const bool readOk = readImuRegisters(imuAddress, QMI8658Register_Timestamp_L, stamp, sizeof(stamp)) &&
                        readImuRegisters(imuAddress, QMI8658Register_Ax_L, raw, sizeof(raw));
    const uint32_t timestamp = uint32_t(stamp[0]) | (uint32_t(stamp[1]) << 8) |
                               (uint32_t(stamp[2]) << 16);
    const bool fresh = readOk && (!haveImuTimestamp || timestamp != lastImuTimestamp);
    if (haveImuTimestamp && now - lastImuSuccessAtMs > nina::GravityEstimator::kMaxSampleAgeMs)
      haveMovementGravity = false;
    if (!fresh && now - lastImuSuccessAtMs > 100) haveMovementGravity = false;
    if (fresh) {
      haveImuTimestamp = true;
      lastImuTimestamp = timestamp;
      lastImuSuccessAtMs = now;
      for (int i = 0; i < 3; ++i) {
        const int16_t acc = int16_t(uint16_t(raw[2*i]) | (uint16_t(raw[2*i+1]) << 8));
        const int16_t gyr = int16_t(uint16_t(raw[6+2*i]) | (uint16_t(raw[7+2*i]) << 8));
        acceleration[i] = acc * (1000.0f / 4096.0f);
        gyro[i] = gyr / 64.0f;
      }
      gravity = gravityEstimator.update(timestamp, {acceleration[0], acceleration[1], acceleration[2]},
                                        {gyro[0], gyro[1], gyro[2]}, now);
    }
    if (gravity.x != 0 || gravity.y != 0 || gravity.z != 0) {
      movementGravity = gravity;
      movementGravityAtMs = now;
      haveMovementGravity = true;
    }
    if (fresh && haveMovementGravity && now - movementGravityAtMs < 2000) {
      accelerationG = (acceleration[0] * movementGravity.x + acceleration[1] * movementGravity.y +
                       acceleration[2] * movementGravity.z) / 1000.0f;
      motionValid = isfinite(accelerationG);
      rollDegrees = atan2f(movementGravity.x, movementGravity.z) * 57.2957795f;
    }
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
  nina::WearableInput input(now, nina::touchIsDown(), expression,
      nina::BatterySnapshot(nina::batteryMillivolts(), false, false, 0, deviceId,
                            true, false, "prototype-13-14"), localIdentity);
  input.motionValid = motionValid;
  input.rollDegrees = rollDegrees;
  input.verticalAccelerationG = accelerationG;
  nina::BlePacket packets[8];
  nina::DiscoveryObservation observations[8];
  size_t count = 0;
  while (count < 8 && discovery.receive(packets[count])) {
    const auto& packet = packets[count];
    observations[count++] = {packet.data, sizeof(packet.data), packet.rssi, packet.receivedMs};
  }
  input.elapsedMs = millis();
  const auto heading = compass.heading(input.elapsedMs);
  input.headingValid = heading.valid;
  input.headingDegrees = heading.degrees;
  auto output = behavior.update(input, observations, count);
  if (radioReady) {
    radio.update(input.elapsedMs);
    nina::RadioPacket packet;
    while (radio.receive(packet)) {
      input.elapsedMs = millis();
      if (input.elapsedMs - packet.receivedMs > 250) continue;
      const auto currentHeading = compass.heading(input.elapsedMs);
      input.headingValid = currentHeading.valid;
      input.headingDegrees = currentHeading.degrees;
      input.radioPeerId = packet.peer;
      input.radioRssi = packet.rssi;
      output = behavior.update(input, nullptr, 0);
    }
  }
  if (!displayReady) return;
  if (output.screen == nina::ScreenName::Face) renderer.render(surface, output.face, 240, 240);
  else renderSettings(output.settings);
  surface.present();
}
