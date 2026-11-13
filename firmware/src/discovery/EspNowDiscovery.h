#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>
#include "BeaconProtocol.h"

namespace nina {
constexpr uint8_t kPeerWifiChannel = 1;
constexpr uint32_t kBeaconPeriodMs = 250;

class EspNowDiscovery {
 public:
  bool begin(uint64_t identity) {
    if (!queue_) queue_ = xQueueCreate(8, sizeof(RadioPacket));
    if (!queue_) return false;
    encodeBeacon(payload_, BeaconType::Direction, identity);
    if (!WiFi.mode(WIFI_STA)) return false;
    if (esp_wifi_set_channel(kPeerWifiChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
        esp_now_init() != ESP_OK) { WiFi.mode(WIFI_OFF); return false; }
    esp_now_peer_info_t peer = {};
    memset(peer.peer_addr, 0xff, 6);
    peer.channel = kPeerWifiChannel;
    peer.ifidx = WIFI_IF_STA;
    if (esp_now_add_peer(&peer) != ESP_OK) { end(); return false; }
    instance() = this;
    wifi_promiscuous_filter_t filter = {};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    if (esp_wifi_set_promiscuous_filter(&filter) != ESP_OK ||
        esp_wifi_set_promiscuous_rx_cb(received) != ESP_OK ||
        esp_wifi_set_promiscuous(true) != ESP_OK) { end(); return false; }
    return true;
  }
  void end() {
    instance() = nullptr;
    esp_wifi_set_promiscuous(false);
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    RadioPacket packet;
    while (receive(packet)) {}
  }
  void update(uint32_t now) {
    if (now - lastSentMs_ < kBeaconPeriodMs) return;
    lastSentMs_ = now;
    static const uint8_t broadcast[] = {255,255,255,255,255,255};
    if (esp_now_send(broadcast, payload_, sizeof(payload_)) != ESP_OK)
      Serial.println("ESP-NOW beacon send failed");
  }
  bool receive(RadioPacket& packet) { return queue_ && xQueueReceive(queue_, &packet, 0) == pdTRUE; }
 private:
  static EspNowDiscovery*& instance() { static EspNowDiscovery* value = nullptr; return value; }
  static void received(void* buffer, wifi_promiscuous_pkt_type_t type) {
    if (!instance() || type != WIFI_PKT_MGMT || !buffer) return;
    const auto* packet = static_cast<const wifi_promiscuous_pkt_t*>(buffer);
    const uint8_t* f = packet->payload;
    const size_t length = packet->rx_ctrl.sig_len;
    if (packet->rx_ctrl.rx_state || length != 55 || f[0] != 0xd0 || (f[1] & 0xcf) ||
        (f[22] & 0x0f)) return;
    if (f[10] & 1) return;
    for (int i = 4; i < 10; ++i) if (f[i] != 0xff) return;
    for (int i = 16; i < 22; ++i) if (f[i] != 0xff) return;
    if (f[24] != 127 || f[25] != 0x18 || f[26] != 0xfe || f[27] != 0x34 ||
        f[32] != 221 || f[33] != 17 || f[34] != 0x18 || f[35] != 0xfe ||
        f[36] != 0x34 || f[37] != 4 || f[38] != 1) return;
    const uint64_t identity = decodeBeacon(f + 39, kBeaconSize, BeaconType::Direction);
    if (!identity || packet->rx_ctrl.rssi >= 0) return;
    RadioPacket observation = {identity, packet->rx_ctrl.rssi, millis()};
    xQueueSend(instance()->queue_, &observation, 0);
  }
  QueueHandle_t queue_ = nullptr;
  uint8_t payload_[12] = {};
  uint32_t lastSentMs_ = 0;
};
}
