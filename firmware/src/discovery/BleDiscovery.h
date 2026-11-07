#pragma once

#include <BLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

namespace nina {

struct BlePacket {
  uint8_t data[12];
  int rssi;
};

class BleDiscovery : public BLEAdvertisedDeviceCallbacks {
 public:
  bool begin(uint64_t identity) {
    queue_ = xQueueCreate(8, sizeof(BlePacket));
    if (!queue_) return false;
    BLEDevice::init("");
    uint8_t data[] = {0xff, 0xff, 'N', 'I', 'N', 'A', 1, 1, 0, 0, 0, 0, 0, 0};
    for (int i = 13; i >= 8; --i) { data[i] = identity & 0xff; identity >>= 8; }
    BLEAdvertisementData advertisement;
    advertisement.setFlags(0x06);
    advertisement.setManufacturerData(std::string(reinterpret_cast<char*>(data), sizeof(data)));
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->setAdvertisementData(advertisement);
    advertising->start();
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->setActiveScan(false);
    scan->setInterval(100);
    scan->setWindow(80);
    return scan->start(0, scanComplete, false);
  }

  bool receive(BlePacket& packet) { return queue_ && xQueueReceive(queue_, &packet, 0) == pdTRUE; }

  void onResult(BLEAdvertisedDevice device) override {
    if (!device.haveManufacturerData() || !device.haveRSSI()) return;
    const std::string data = device.getManufacturerData();
    if (data.size() != 14 || uint8_t(data[0]) != 0xff || uint8_t(data[1]) != 0xff ||
        data.compare(2, 4, "NINA") != 0 || uint8_t(data[6]) != 1 || uint8_t(data[7]) != 1)
      return;
    BlePacket packet;
    memcpy(packet.data, data.data() + 2, sizeof(packet.data));
    packet.rssi = device.getRSSI();
    xQueueSend(queue_, &packet, 0);
  }

 private:
  static void scanComplete(BLEScanResults) {}
  QueueHandle_t queue_ = nullptr;
};

}  // namespace nina
