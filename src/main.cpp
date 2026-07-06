#include <Arduino.h>
#include "esp_task_wdt.h"

#include "config.h"
#include "battery.h"
#include "scale.h"
#include "display_mgr.h"
#include "bt_handler.h"

// =============================================================
//  ScaleReader v2 — Production Firmware
//  Hardware : ESP32 + 2× Samsung 18650 parallel + BMS + 5V booster
//  Display  : SSD1306 OLED 128×64
//  Weight   : RS232 scale via MAX3232 → UART2
//  Comms    : Bluetooth Classic
// =============================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ScaleReader v2 BOOT ===");

    // --- Watchdog: reboot if loop stalls for WDT_TIMEOUT_S seconds ---
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    // --- GPIO ---
    pinMode(LED_HIJAU, OUTPUT);
    pinMode(LED_MERAH, OUTPUT);
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);   // red = waiting

    // --- Subsystems ---
    battery_init();
    scale_init();
    display_init();
    bt_init();

    Serial.println("[READY] Send GET_WEIGHT via BT to query weight");
}

void loop() {
    esp_task_wdt_reset();   // feed watchdog

    scale_tick();           // read RS232 bytes (non-blocking)
    bt_tick();              // handle BT commands + detect connect/disconnect

    // Battery: one ADC sample per tick; result ready after ADC_SAMPLES ticks
    if (battery_tick()) {
        display_set_battery(battery_get_percent());
    }

    // Push latest weight to display state every loop
    display_set_weight(scale_get_weight());

    display_tick();         // refresh OLED (throttled to 200 ms)
}