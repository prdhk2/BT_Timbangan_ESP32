#pragma once
#include <Arduino.h>

void    battery_init();
// Returns true when a full 8-sample reading is ready and batteryPercent updated.
// Call every loop iteration; gated internally to BATTERY_READ_INTERVAL_MS.
bool    battery_tick();
float   battery_get_percent();