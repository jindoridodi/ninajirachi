#pragma once
#include "Arduino.h"
constexpr int WIFI_STA = 1;
struct WiFiType { bool mode(int) { return true; } };
extern WiFiType WiFi;
