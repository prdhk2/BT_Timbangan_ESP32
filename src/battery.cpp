#include "battery.h"
#include "config.h"

static float    s_batteryPercent  = 0.0f;
static int32_t  s_accumulator     = 0;   // int32_t: explicit, avoids long sign mismatch
static uint8_t  s_sampleCount     = 0;
static bool     s_sampling        = false;
static unsigned long s_lastTrigger = 0UL;

// Map voltage to % — 2S Li-ion pack: 6.0 V (0%) … 8.4 V (100%)
// Curve mirrors single-cell profile scaled ×2:
//   8.40 V = 4.20/cell = 100%
//   8.00 V = 4.00/cell =  80%
//   7.70 V = 3.85/cell =  50%
//   7.40 V = 3.70/cell =  25%
//   7.00 V = 3.50/cell =  10%
//   6.60 V = 3.30/cell =   0% (cutoff — BMS should trip before this)
static float voltageToPercent(float v) {
    if      (v >= 8.40f) return 100.0f;
    else if (v >= 8.00f) return  80.0f + (v - 8.00f) * 50.0f;
    else if (v >= 7.70f) return  50.0f + (v - 7.70f) * 100.0f;
    else if (v >= 7.40f) return  25.0f + (v - 7.40f) * 83.3f;
    else if (v >= 7.00f) return  10.0f + (v - 7.00f) * 37.5f;
    else if (v >= 6.60f) return          (v - 6.60f) * 25.0f;
    else                 return   0.0f;
}

void battery_init() {
    pinMode(BATTERY_PIN, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);  // pin-scoped only
    // Reset sampling state — safe against double-init on soft-reset
    s_accumulator = 0;
    s_sampleCount = 0;
    s_lastTrigger = millis();
    s_sampling    = true;   // start first reading immediately at boot
}

bool battery_tick() {
    unsigned long now = millis();

    // Gate: start a new sampling burst every BATTERY_READ_INTERVAL_MS
    if (!s_sampling && (now - s_lastTrigger >= BATTERY_READ_INTERVAL_MS)) {
        s_sampling    = true;
        s_lastTrigger = now;
        s_accumulator = 0;
        s_sampleCount = 0;
    }

    if (!s_sampling) return false;

    // Take one sample this tick
    s_accumulator += analogRead(BATTERY_PIN);
    s_sampleCount++;

    if (s_sampleCount < ADC_SAMPLES) return false;  // burst not complete yet

    // Burst complete — compute result
    float raw    = (float)s_accumulator / (float)ADC_SAMPLES;
    float vPin   = (raw / 4095.0f) * 3.3f * ADC_CALIBRATION;
    float vBatt  = vPin * (R1 + R2) / R2;

    s_batteryPercent = constrain(voltageToPercent(vBatt), 0.0f, 100.0f);
    Serial.printf("[BAT] %.2fV | %.1f%%\n", vBatt, s_batteryPercent);

    s_sampling    = false;
    s_accumulator = 0;
    s_sampleCount = 0;
    return true;
}

float battery_get_percent() {
    return s_batteryPercent;
}