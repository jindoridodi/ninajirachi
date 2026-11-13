#pragma once
#include "Arduino.h"
#include <string>
using esp_gap_ble_cb_event_t = int;
struct esp_ble_gap_cb_param_t { struct { int status; } adv_data_raw_cmpl, adv_start_cmpl; };
struct esp_ble_adv_params_t { int adv_int_min, adv_int_max, adv_type, own_addr_type, channel_map, adv_filter_policy; };
constexpr int ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT = 1, ESP_GAP_BLE_ADV_START_COMPLETE_EVT = 2;
constexpr int ESP_BT_STATUS_SUCCESS = 0, ADV_TYPE_IND = 0, BLE_ADDR_TYPE_PUBLIC = 0, ADV_CHNL_ALL = 0, ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY = 0;
struct BleStartup {
  void (*callback)(int, esp_ble_gap_cb_param_t*) = nullptr;
  std::string payload;
  int configResult = ESP_OK, startResult = ESP_OK, starts = 0;
  int advMin = 0, advMax = 0;
};
extern BleStartup bleStartup;
inline int esp_ble_gap_config_adv_data_raw(uint8_t* data, size_t size) {
  bleStartup.payload.assign(reinterpret_cast<char*>(data), size);
  return bleStartup.configResult;
}
inline int esp_ble_gap_start_advertising(esp_ble_adv_params_t* params) {
  bleStartup.advMin = params->adv_int_min; bleStartup.advMax = params->adv_int_max;
  ++bleStartup.starts;
  return bleStartup.startResult;
}
struct BLEAdvertisedDevice {
  std::string data; int rssi = -50; bool hasRssi = true;
  bool haveManufacturerData() { return !data.empty(); }
  bool haveRSSI() { return hasRssi; }
  std::string getManufacturerData() { return data; }
  int getRSSI() { return rssi; }
};
struct BLEAdvertisedDeviceCallbacks { virtual ~BLEAdvertisedDeviceCallbacks() = default; virtual void onResult(BLEAdvertisedDevice) {} };
struct BLEAdvertisementData { std::string data; void setFlags(int) {} void setManufacturerData(std::string d) { data = d; } std::string getPayload() { return data; } };
struct BLEScanResults {};
struct BLEScan {
  BLEAdvertisedDeviceCallbacks* callback = nullptr;
  int interval = 0, window = 0;
  void setAdvertisedDeviceCallbacks(BLEAdvertisedDeviceCallbacks* cb, bool) { callback = cb; }
  void setActiveScan(bool) {} void setInterval(int value) { interval = value; } void setWindow(int value) { window = value; }
  bool start(int, void (*)(BLEScanResults), bool) { return true; }
};
struct BLEDevice {
  static void init(const char*) {} static void setCustomGapHandler(void (*cb)(int, esp_ble_gap_cb_param_t*)) { bleStartup.callback = cb; }
  static BLEScan* getScan() { static BLEScan scan; return &scan; }
};
