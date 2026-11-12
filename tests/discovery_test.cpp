#include "../firmware/src/discovery/EspNowDiscovery.h"
#include "../firmware/src/discovery/BleDiscovery.h"
#include <cassert>
#include <algorithm>

SerialType Serial;
WiFiType WiFi;
void (*radioCallback)(void*, wifi_promiscuous_pkt_type_t) = nullptr;

int main() {
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
  assert(ble.begin(1));
  BLEAdvertisedDevice device;
  const char data[] = {char(0xff),char(0xff),'N','I','N','A',1,1,0,0,0,0,0,2};
  device.data.assign(data, sizeof(data));
  nina::BlePacket received{};
  device.data.pop_back(); BLEDevice::getScan()->callback->onResult(device);
  device.data.assign(data, sizeof(data)); device.data[6] = 2; BLEDevice::getScan()->callback->onResult(device);
  assert(!ble.receive(received));
  device.data[6] = 1;
  for (int i = 0; i < 9; ++i) BLEDevice::getScan()->callback->onResult(device);
  for (int i = 0; i < 8; ++i) {
    assert(ble.receive(received));
    assert(received.data[11] == 2 && received.rssi == -50 && received.receivedMs == 123);
  }
  assert(!ble.receive(received));
}
