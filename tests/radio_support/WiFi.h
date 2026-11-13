#pragma once
#include "Arduino.h"
constexpr int WIFI_STA = 1, WIFI_OFF = 0;
struct WiFiType { int current = WIFI_OFF; bool mode(int value) { current = value; return true; } };
extern WiFiType WiFi;
