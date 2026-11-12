#include "../firmware/src/discovery/EspNowDiscovery.h"
#include "../firmware/src/discovery/BleDiscovery.h"
#include <cassert>
#include <algorithm>

SerialType Serial;
BleStartup bleStartup;
WiFiType WiFi;
void (*radioCallback)(void*, wifi_promiscuous_pkt_type_t) = nullptr;

int main() {
  const uint8_t known[] = {'N','I','N','A',1,1,0x12,0x34,0x56,0x78,0x9a,0xbc};
  uint8_t encoded[12];
  nina::encodeBeacon(encoded, nina::BeaconType::Discovery, 0x123456789abcULL);
  assert(std::equal(encoded, encoded + 12, known));
  assert(nina::decodeBeacon(known, 12, nina::BeaconType::Discovery) == 0x123456789abcULL);
  assert(!nina::decodeBeacon(nullptr, 12, nina::BeaconType::Discovery));
  assert(!nina::decodeBeacon(known, 11, nina::BeaconType::Discovery));
  assert(!nina::decodeBeacon(known, 12, nina::BeaconType::Direction));
  for (int i = 0; i < 6; ++i) {
    std::copy(known, known + 12, encoded);
    encoded[i] ^= 1;
    assert(!nina::decodeBeacon(encoded, 12, nina::BeaconType::Discovery));
  }
  nina::encodeBeacon(encoded, nina::BeaconType::Direction, 0x123456789abcULL);
  assert(encoded[5] == 2 && nina::decodeBeacon(encoded, 12, nina::BeaconType::Direction) == 0x123456789abcULL);
  nina::encodeBeacon(encoded, nina::BeaconType::Discovery, 0);
  assert(!nina::decodeBeacon(encoded, 12, nina::BeaconType::Discovery));
  nina::EspNowDiscovery radio;
  assert(radio.begin(1) && radioCallback);
  wifi_promiscuous_pkt_t frame{};
  frame.rx_ctrl.sig_len = 55; frame.rx_ctrl.rssi = -45;
  auto* f = frame.payload;
  f[0] = 0xd0;
  std::fill(f + 4, f + 10, 0xff);
  std::fill(f + 16, f + 22, 0xff);
  const uint8_t body[] = {127,0x18,0xfe,0x34,0,0,0,0,221,17,0x18,0xfe,0x34,4,1,'N','I','N','A',1,2,0,0,0,0,0,2};
  std::copy(body, body + sizeof(body), f + 24);
  nina::RadioPacket packet{};
  frame.rx_ctrl.sig_len = 54; radioCallback(&frame, WIFI_PKT_MGMT);
  frame.rx_ctrl.sig_len = 55; f[43] = 2; radioCallback(&frame, WIFI_PKT_MGMT);
  assert(!radio.receive(packet));
  f[43] = 1;
  for (int i = 0; i < 9; ++i) radioCallback(&frame, WIFI_PKT_MGMT);
  for (int i = 0; i < 8; ++i) {
    assert(radio.receive(packet));
    assert(packet.peer == 2 && packet.rssi == -45 && packet.receivedMs == 123);
  }
  assert(!radio.receive(packet));

  nina::BleDiscovery ble;
  bleStartup.configResult = 1;
  assert(!ble.begin(1));
  bleStartup.configResult = ESP_OK;
  assert(ble.begin(0x123456789abcULL));
  const unsigned char expected[] = {255,255,'N','I','N','A',1,1,0x12,0x34,0x56,0x78,0x9a,0xbc};
  assert(bleStartup.payload == std::string(reinterpret_cast<const char*>(expected), sizeof(expected)));
  assert(bleStartup.callback && bleStartup.starts == 0);
  esp_ble_gap_cb_param_t event{};
  event.adv_data_raw_cmpl.status = 1;
  int logs = Serial.messages;
  bleStartup.callback(ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT, &event);
  assert(bleStartup.starts == 0 && Serial.messages == logs + 1);
  event.adv_data_raw_cmpl.status = ESP_BT_STATUS_SUCCESS;
  bleStartup.startResult = 1;
  logs = Serial.messages;
  bleStartup.callback(ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT, &event);
  assert(bleStartup.starts == 1 && Serial.messages == logs + 1);
  bleStartup.startResult = ESP_OK;
  bleStartup.callback(ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT, &event);
  assert(bleStartup.starts == 2);
  event.adv_start_cmpl.status = 1;
  logs = Serial.messages;
  bleStartup.callback(ESP_GAP_BLE_ADV_START_COMPLETE_EVT, &event);
  assert(Serial.messages == logs + 1);
  event.adv_start_cmpl.status = ESP_BT_STATUS_SUCCESS;
  logs = Serial.messages;
  bleStartup.callback(ESP_GAP_BLE_ADV_START_COMPLETE_EVT, &event);
  assert(Serial.messages == logs);

  BLEAdvertisedDevice device;
  const char data[] = {char(0xff),char(0xff),'N','I','N','A',1,1,0,0,0,0,0,2};
  device.data.assign(data, sizeof(data));
  nina::DiscoveryObservation received{};
  device.data.pop_back(); BLEDevice::getScan()->callback->onResult(device);
  device.data.assign(data, sizeof(data)); device.data[6] = 2; BLEDevice::getScan()->callback->onResult(device);
  assert(!ble.receive(received));
  device.data[6] = 1;
  for (int i = 0; i < 9; ++i) BLEDevice::getScan()->callback->onResult(device);
  for (int i = 0; i < 8; ++i) {
    assert(ble.receive(received));
    assert(received.peerId == 2 && received.rssi == -50 && received.receivedMs == 123);
  }
  assert(!ble.receive(received));
}
