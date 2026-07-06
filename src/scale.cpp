#include "scale.h"
#include "config.h"

// UART2 — RS232 via MAX3232
static HardwareSerial s_rs232(2);
static float          s_lastWeight = 0.0f;
static bool           s_hasData    = false;  // true once a valid frame is parsed

// Extract first numeric token from null-terminated buffer.
// Returns false and leaves out unchanged if no valid number is found.
// Requires at least one digit (rejects bare '-' sign frames).
static bool parseFrame(const char *buf, float &out) {
    char    num[16];
    uint8_t ni        = 0;
    bool    foundNum  = false;
    bool    foundDot  = false;
    bool    foundNeg  = false;

    for (const char *p = buf; *p && ni < 15; p++) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            num[ni++] = c;
            foundNum  = true;
        } else if (c == '.' && !foundDot && foundNum) {
            num[ni++] = c;
            foundDot  = true;
        } else if (c == '-' && !foundNum && !foundNeg) {
            num[ni++] = c;
            foundNeg  = true;
        } else if (foundNum) {
            break;  // stop on first non-numeric after digits
        }
    }

    // Require at least one actual digit — reject bare '-' or empty tokens
    if (!foundNum) return false;

    num[ni] = '\0';
    out = (float)atof(num);
    return true;
}

void scale_init() {
    s_rs232.begin(9600, SERIAL_8N1, WEIGHT_RX_PIN, WEIGHT_TX_PIN);
    Serial.println("[RS232] Scale serial ready (MAX3232)");
}

void scale_tick() {
    static char    buf[RS232_BUF_SIZE];
    static uint8_t idx = 0;

    while (s_rs232.available()) {
        char c = (char)s_rs232.read();

        if (c == '\n') {
            // Strip trailing CR if present (some scales send \r\n)
            if (idx > 0 && buf[idx - 1] == '\r') idx--;
            buf[idx] = '\0';

            if (idx > 0) {
                float w;
                if (parseFrame(buf, w) && w >= 0.0f) {
                    s_lastWeight = w;
                    s_hasData    = true;
                    Serial.printf("[SCALE] %.2f kg\n", s_lastWeight);
                }
            }
            idx = 0;

        } else if (idx < RS232_BUF_SIZE - 1) {
            buf[idx++] = c;
        } else {
            // Frame overflow — discard and reset
            idx = 0;
            Serial.println("[SCALE] Frame overflow — discarded");
        }
    }
}

float scale_get_weight() {
    return s_lastWeight;
}

bool scale_has_data() {
    return s_hasData;
}