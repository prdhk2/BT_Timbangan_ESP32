#include "display_mgr.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Konstruktor U8g2 menggunakan Hardware I2C. 
// JIKA LAYAR TETAP BERGARIS/GLITCH, GANTI "SSD1306" MENJADI "SH1106" di bawah ini:
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static float            s_weight        = 0.0f;
static float            s_battPct       = 0.0f;
static bool             s_btConnected   = false;
static NotificationType s_notif         = NOTIF_NONE;
static unsigned long    s_notifStart    = 0UL;
static bool             s_displayOk     = false;

// ---- Battery icon ------------------------------------------------
static void drawBatteryIcon(int x, int y, float pct) {
    bool low = (pct < BATT_LOW_WARN);

    // Gambar bodi baterai (Lebar 22, Tinggi 9)
    u8g2.drawFrame(x, y, 22, 9);
    u8g2.drawBox(x + 22, y + 2, 2, 5); // Kepala baterai (nub)

    // Isi persentase baterai
    int fillPx = map((int)constrain(pct, 0.0f, 100.0f), 0, 100, 0, 20);
    if (fillPx > 0) {
        u8g2.drawBox(x + 1, y + 1, fillPx, 7);
    }

    // Tampilkan teks persentase disamping ikon
    u8g2.setFont(u8g2_font_6x10_tf); // Font kecil yang sangat jelas, tinggi ~7px
    u8g2.setCursor(x + 27, y + 8);   // Koordinat Y disesuaikan dengan baseline font
    u8g2.print((int)pct);
    u8g2.print('%');

    // Kedip tanda seru '!' jika low batt (tiap 500ms)
    if (low && ((millis() / 500UL) & 1U)) {
        u8g2.setCursor(x + 57, y + 8);
        u8g2.print('!');
    }
}

// ---- Status bar (rows 0–12) -------------------------------------
static void drawStatusBar() {
    // Bluetooth Icon (Custom minimal drawing agar rapi)
    int bx = 2, by = 1;
    if (s_btConnected) {
        u8g2.drawTriangle(bx+3, by,   bx+7, by+4, bx+3, by+8);
        u8g2.drawTriangle(bx+3, by,   bx-1, by+4, bx+3, by+8);
        u8g2.setDrawColor(0); // hapus pixel tengah agar membentuk icon BT
        u8g2.drawPixel(bx+3, by+4);
        u8g2.setDrawColor(1); // kembalikan warna putih
    } else {
        u8g2.drawFrame(bx, by, 7, 9); // Kotak indikator putus jika tidak konek
        u8g2.drawLine(bx, by, bx+6, by+8); // Garis silang penanda putus
    }

    drawBatteryIcon(82, 1, s_battPct);

    // Garis pembatas status bar bawah (Baris ke-12)
    u8g2.drawLine(0, 12, 127, 12);
}

// ---- Weight area (rows 13–50) -----------------------------------
static void drawWeightArea() {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 22, "BERAT"); // Label di kiri atas area berat

    char wBuf[12];
    if (s_weight == 0.0f) {
        strcpy(wBuf, "0.00");
    } else {
        dtostrf(s_weight, 1, 2, wBuf);
    }

    // Gunakan Font Logis Ukuran Besar yang sangat tajam dan anti-aliased padat
    u8g2.setFont(u8g2_font_logisoso20_tf); // Tinggi font 20px, sangat pas untuk angka timbangan
    
    // Auto-centering otomatis berbasis pixel dari library U8g2 (Garansi presisi ditengah)
    int totalW = u8g2.getStrWidth(wBuf);
    int wx = (128 - totalW) / 2;

    u8g2.drawStr(wx, 46, wBuf); // Tampilkan nilai berat tepat di tengah

    // Label satuan "kg" diletakkan di pojok kanan bawah area berat
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(113, 46, "kg");
}

// ---- Bottom bar (rows 51–63) ------------------------------------
static void drawBottomBar() {
    // Garis pembatas bottom bar (Baris ke-51)
    u8g2.drawLine(0, 51, 127, 51);

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 62, "v2.0");

    if (s_btConnected) {
        u8g2.drawStr(66, 62, "TERHUBUNG");
    } else {
        u8g2.drawStr(60, 62, "MENUNGGU...");
    }
}

// ---- Full notification screen -----------------------------------
static void renderNotification(NotificationType notif) {
    u8g2.clearBuffer();
    
    bool ok = (notif == NOTIF_SEND_SUCCESS);
    const int cx = 64, cy = 24, r = 12;
    u8g2.drawCircle(cx, cy, r);

    if (ok) {
        // Ikon Centang (Checkmark)
        u8g2.drawLine(cx-5, cy, cx-1, cy+4);
        u8g2.drawLine(cx-1, cy+4, cx+6, cy-5);
    } else {
        // Ikon Silang (X)
        u8g2.drawLine(cx-4, cy-4, cx+4, cy+4);
        u8g2.drawLine(cx+4, cy-4, cx-4, cy+4);
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    if (ok) {
        int tw = u8g2.getStrWidth("DATA TERKIRIM");
        u8g2.drawStr((128 - tw) / 2, 48, "DATA TERKIRIM");
    } else {
        int tw = u8g2.getStrWidth("PENGIRIMAN GAGAL");
        u8g2.drawStr((128 - tw) / 2, 48, "PENGIRIMAN GAGAL");
    }
    
    u8g2.drawLine(15, 56, 113, 56);
    u8g2.sendBuffer();
}

static void renderMainScreen() {
    u8g2.clearBuffer();
    drawStatusBar();
    drawWeightArea();
    drawBottomBar();
    u8g2.sendBuffer();
}

// ---- Public API -------------------------------------------------

void display_init() {
    // Alamat I2C otomatis diatur oleh U8g2 (default 0x3C). 
    if (!u8g2.begin()) {
        Serial.println("[OLED] INIT FAILED — check wiring / address");
        s_displayOk = false;
        return;
    }
    s_displayOk = true;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    Serial.println("[OLED] Ready (U8g2 Mode)");
}

void display_set_weight(float w)           { s_weight      = w;   }
void display_set_battery(float pct)        { s_battPct     = pct; }
void display_set_bt_connected(bool connected) { s_btConnected = connected; }

void display_show_notification(NotificationType notif) {
    if (!s_displayOk) return;
    s_notif      = notif;
    s_notifStart = millis();
    renderNotification(notif);
}

void display_tick() {
    if (!s_displayOk) return;
    static unsigned long s_lastRefresh = 0UL;
    unsigned long now = millis();
    if (now - s_lastRefresh < DISPLAY_UPDATE_INTERVAL_MS) return;
    s_lastRefresh = now;

    if (s_notif != NOTIF_NONE) {
        if (now - s_notifStart >= NOTIF_DURATION_MS) {
            s_notif = NOTIF_NONE;
            renderMainScreen();
        }
    } else {
        renderMainScreen();
    }
}
