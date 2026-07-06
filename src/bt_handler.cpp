#include "bt_handler.h"
#include "config.h"
#include "scale.h"
#include "display_mgr.h"
#include "BluetoothSerial.h"

static BluetoothSerial s_bt;
static bool            s_connected = false;

void bt_init() {
    s_bt.begin("ScaleReader");
    Serial.println("[BT] Device name: ScaleReader");
}

void bt_tick() {
    // --- Connection state change detection ---
    bool nowConnected = s_bt.hasClient();
    if (nowConnected != s_connected) {
        s_connected = nowConnected;
        display_set_bt_connected(s_connected);
        digitalWrite(LED_HIJAU, s_connected ? HIGH : LOW);
        digitalWrite(LED_MERAH, s_connected ? LOW  : HIGH);
        Serial.println(s_connected ? "[BT] CONNECTED" : "[BT] WAITING");
    }

    // --- Read incoming command bytes ---
    static char    buf[BT_CMD_BUF_SIZE];
    static uint8_t idx = 0;

    while (s_bt.available()) {
        char c = (char)s_bt.read();

        if (c == '\n') {
            // Strip trailing CR
            if (idx > 0 && buf[idx - 1] == '\r') idx--;
            buf[idx] = '\0';

            Serial.print("[CMD] ");
            Serial.println(buf);

            if (strcmp(buf, "GET_WEIGHT") == 0) {
                char resp[BT_RESP_BUF_SIZE];

                if (!scale_has_data()) {
                    strncpy(resp, "NO_DATA", sizeof(resp));
                } else {
                    float w = scale_get_weight();
                    dtostrf(w, 1, 2, resp);
                    // Append " kg" safely
                    size_t len = strlen(resp);
                    if (len + 3 < sizeof(resp)) {
                        resp[len]   = ' ';
                        resp[len+1] = 'k';
                        resp[len+2] = 'g';
                        resp[len+3] = '\0';
                    }
                }

                bool ok = s_bt.println(resp);
                Serial.print(ok ? "[SEND] " : "[SEND FAILED] ");
                Serial.println(resp);
                display_show_notification(ok ? NOTIF_SEND_SUCCESS : NOTIF_SEND_FAILED);
            }

            idx = 0;

        } else if (idx < BT_CMD_BUF_SIZE - 1) {
            buf[idx++] = c;
        } else {
            // Overlong command — discard
            idx = 0;
        }
    }
}

bool bt_is_connected() {
    return s_connected;
}