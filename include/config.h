#pragma once

// ===================== PIN DEFINITIONS =====================
#define LED_HIJAU           18
#define LED_MERAH           19
#define BATTERY_PIN         34
#define WEIGHT_RX_PIN       16
// UART2 default TX — pin left floating/unconnected on board.
// Do NOT use -1: some ESP32 Arduino core versions silently assert on txPin=-1.
#define WEIGHT_TX_PIN       17

// ===================== BATTERY =============================
// 2S Li-ion pack: 2× 18650 in SERIES via BMS
// Full: 8.40 V (4.20V/cell) | Cutoff: 6.60 V (3.30V/cell)
// Divider: R1=200kΩ (2×100k series from BAT+), R2=100kΩ to GND → GPIO34
// Ratio: (R1+R2)/R2 = 3.0  →  max readable = 3.3V × 3.0 = 9.9V  ✓
#define R1                  200000.0f
#define R2                  100000.0f
#define ADC_CALIBRATION     1.0f        // tune after measuring with multimeter
#define BATT_FULL_VOLTAGE   8.40f
#define BATT_EMPTY_VOLTAGE  6.60f
#define BATT_LOW_WARN       15.0f       // % below this → blink "!"
#define ADC_SAMPLES         8

// ===================== OLED ================================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDR           0x3C

// ===================== TIMING ==============================
#define BATTERY_READ_INTERVAL_MS    5000UL
#define DISPLAY_UPDATE_INTERVAL_MS   200UL
#define NOTIF_DURATION_MS           2000UL

// ===================== BUFFERS =============================
#define RS232_BUF_SIZE      48
#define BT_CMD_BUF_SIZE     72
#define BT_RESP_BUF_SIZE    24

// ===================== WATCHDOG ============================
#define WDT_TIMEOUT_S       10