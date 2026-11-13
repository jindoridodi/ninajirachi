#pragma once
#include "Arduino.h"
constexpr int WIFI_IF_STA = 1;
struct esp_now_peer_info_t { uint8_t peer_addr[6]; int channel; int ifidx; };
inline int esp_now_deinit() { return ESP_OK; }
inline int esp_now_init() { return ESP_OK; }
inline int esp_now_add_peer(esp_now_peer_info_t*) { return ESP_OK; }
inline int esp_now_send(const uint8_t*, const uint8_t*, size_t) { return ESP_OK; }
