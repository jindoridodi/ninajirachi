#pragma once

#include <BLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>
#include <atomic>
#include "BeaconProtocol.h"

namespace nina {

class BleDiscovery : public BLEAdvertisedDeviceCallbacks {
 public:
  bool begin(uint64_t identity) {
    identity_ = identity;
    if (!queue_) queue_ = xQueueCreate(8, sizeof(DiscoveryObservation));
    if (!queue_) return false;
    BLEDevice::init("");
    BLEDevice::setCustomGapHandler(gapEvent);
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this, true);
    scan->setActiveScan(false);
    scan->setInterval(100);
    scan->setWindow(40);
    initialized_ = true;
    update(millis(), true);
    return scanReady_ && advertising_.load() != Failed;
  }

  void update(uint32_t now, bool force = false) {
    if (!force && now - lastAttemptMs_ < 3000) return;
    lastAttemptMs_ = now;
    if (!initialized_) { begin(identity_); return; }
    if (scanFailed_.exchange(false)) { BLEDevice::getScan()->stop(); scanReady_ = false; }
    if (!scanReady_) scanReady_ = BLEDevice::getScan()->start(0, scanComplete, false);
    if (advertising_.load() == Failed) {
      uint8_t data[14] = {0xff, 0xff};
      encodeBeacon(data + 2, BeaconType::Discovery, identity_);
      BLEAdvertisementData advertisement;
      advertisement.setFlags(0x06);
      advertisement.setManufacturerData(std::string(reinterpret_cast<char*>(data), sizeof(data)));
      const std::string payload = advertisement.getPayload();
      advertising_ = Pending;
      if (esp_ble_gap_config_adv_data_raw(
          reinterpret_cast<uint8_t*>(const_cast<char*>(payload.data())), payload.size()) != ESP_OK)
        advertising_ = Failed;
    }
  }

  bool receive(DiscoveryObservation& packet) { return queue_ && xQueueReceive(queue_, &packet, 0) == pdTRUE; }

  void onResult(BLEAdvertisedDevice device) override {
    const uint32_t receivedMs = millis();
    if (!device.haveManufacturerData() || !device.haveRSSI()) return;
    const std::string data = device.getManufacturerData();
    if (data.size() != 14 || uint8_t(data[0]) != 0xff || uint8_t(data[1]) != 0xff) return;
    const uint64_t peer = decodeBeacon(reinterpret_cast<const uint8_t*>(data.data()) + 2,
                                       data.size() - 2, BeaconType::Discovery);
    if (!peer) return;
    DiscoveryObservation packet;
    packet.peerId = peer;
    packet.rssi = device.getRSSI();
    packet.receivedMs = receivedMs;
    xQueueSend(queue_, &packet, 0);
  }

 private:
  static void gapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    if (event == ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT) {
      if (param->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        advertising_ = Failed;
        Serial.printf("BLE advertising data failed: %d\n", int(param->adv_data_raw_cmpl.status));
        return;
      }
      static esp_ble_adv_params_t params = {};
      params.adv_int_min = 0x100;
      params.adv_int_max = 0x120;
      params.adv_type = ADV_TYPE_NONCONN_IND;
      params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
      params.channel_map = ADV_CHNL_ALL;
      params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
      const esp_err_t error = esp_ble_gap_start_advertising(&params);
      if (error != ESP_OK) { advertising_ = Failed; Serial.printf("BLE advertising start request failed: %d\n", int(error)); }
    } else if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT &&
               param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      advertising_ = Failed;
      Serial.printf("BLE advertising start failed: %d\n", int(param->adv_start_cmpl.status));
    } else if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT) {
      advertising_ = Ready;
    } else if (event == ESP_GAP_BLE_SCAN_START_COMPLETE_EVT &&
               param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      scanFailed_ = true;
    }
  }

  static void scanComplete(BLEScanResults) { scanFailed_ = true; }
  enum AdvertisingState { Failed, Pending, Ready };
  static std::atomic<int> advertising_;
  static std::atomic<bool> scanFailed_;
  uint64_t identity_ = 0;
  uint32_t lastAttemptMs_ = 0;
  bool initialized_ = false, scanReady_ = false;
  QueueHandle_t queue_ = nullptr;
};

std::atomic<int> BleDiscovery::advertising_{BleDiscovery::Failed};
std::atomic<bool> BleDiscovery::scanFailed_{false};

}  // namespace nina
