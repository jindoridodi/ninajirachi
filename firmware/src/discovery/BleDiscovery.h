#pragma once

#include <BLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

namespace nina {

struct BlePacket {
  uint8_t data[12];
  int rssi;
  uint32_t receivedMs;
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
    BLEDevice::setCustomGapHandler(gapEvent);
    const std::string payload = advertisement.getPayload();
    const esp_err_t advertisingError = esp_ble_gap_config_adv_data_raw(
        reinterpret_cast<uint8_t*>(const_cast<char*>(payload.data())), payload.size());
    if (advertisingError != ESP_OK) {
      Serial.printf("BLE advertising data request failed: %d\n", int(advertisingError));
      return false;
    }
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->setActiveScan(false);
    scan->setInterval(100);
    scan->setWindow(80);
    return scan->start(0, scanComplete, false);
  }

  bool receive(BlePacket& packet) { return queue_ && xQueueReceive(queue_, &packet, 0) == pdTRUE; }

  void onResult(BLEAdvertisedDevice device) override {
    const uint32_t receivedMs = millis();
    if (!device.haveManufacturerData() || !device.haveRSSI()) return;
    const std::string data = device.getManufacturerData();
    if (data.size() != 14 || uint8_t(data[0]) != 0xff || uint8_t(data[1]) != 0xff ||
        data.compare(2, 4, "NINA") != 0 || uint8_t(data[6]) != 1 || uint8_t(data[7]) != 1)
      return;
    BlePacket packet;
    memcpy(packet.data, data.data() + 2, sizeof(packet.data));
    packet.rssi = device.getRSSI();
    packet.receivedMs = receivedMs;
    xQueueSend(queue_, &packet, 0);
  }

 private:
  static void gapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    if (event == ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT) {
      if (param->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        Serial.printf("BLE advertising data failed: %d\n", int(param->adv_data_raw_cmpl.status));
        return;
      }
      static esp_ble_adv_params_t params = {};
      params.adv_int_min = 0x20;
      params.adv_int_max = 0x40;
      params.adv_type = ADV_TYPE_IND;
      params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
      params.channel_map = ADV_CHNL_ALL;
      params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
      const esp_err_t error = esp_ble_gap_start_advertising(&params);
      if (error != ESP_OK) Serial.printf("BLE advertising start request failed: %d\n", int(error));
    } else if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT &&
               param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      Serial.printf("BLE advertising start failed: %d\n", int(param->adv_start_cmpl.status));
    }
  }

  static void scanComplete(BLEScanResults) {}
  QueueHandle_t queue_ = nullptr;
};

}  // namespace nina
