#pragma once
#include "Arduino.h"
constexpr int WIFI_SECOND_CHAN_NONE = 0, WIFI_PROMIS_FILTER_MASK_MGMT = 1;
using wifi_promiscuous_pkt_type_t = int;
constexpr int WIFI_PKT_MGMT = 1;
struct wifi_promiscuous_filter_t { int filter_mask; };
struct wifi_promiscuous_pkt_t { struct { int sig_len; int rx_state; int rssi; } rx_ctrl; uint8_t payload[55]; };
extern void (*radioCallback)(void*, wifi_promiscuous_pkt_type_t);
inline int esp_wifi_set_channel(int, int) { return ESP_OK; }
inline int esp_wifi_set_promiscuous_filter(wifi_promiscuous_filter_t*) { return ESP_OK; }
inline int esp_wifi_set_promiscuous_rx_cb(void (*cb)(void*, int)) { radioCallback = cb; return ESP_OK; }
inline int esp_wifi_set_promiscuous(bool) { return ESP_OK; }
