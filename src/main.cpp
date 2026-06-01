#include <Arduino.h>
#include "BluetoothSerial.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

BluetoothSerial SerialBT;

// ============ PIN DEFINITIONS ============
#define LED_HIJAU   18
#define LED_MERAH   19
#define BATTERY_PIN 34

// ============ BATTERY PARAMETERS (3.7V Li-Ion) ============
// Pilih salah satu kombinasi yang kamu punya:

// OPSI 1: Paling stabil (100k + 47k)
#define R1 100000.0
#define R2 47000.0

// OPSI 2: Kalau punya 2 resistor sama (47k + 47k)
// #define R1 47000.0
// #define R2 47000.0

// OPSI 3: Kalau punya 220k + 100k
// #define R1 220000.0
// #define R2 100000.0

// Range baterai 3.7V Li-Ion
#define BATT_FULL_VOLTAGE 4.2    // 100%
#define BATT_EMPTY_VOLTAGE 3.0   // 0%

// ============ OLED ============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ============ GLOBAL ============
String btCommand = "";
float lastWeight = 0.0;
float batteryPercent = 0.0;
bool bluetoothConnected = false;

enum NotificationType {
  NOTIF_NONE,
  NOTIF_SEND_SUCCESS,
  NOTIF_SEND_FAILED
};

NotificationType currentNotif = NOTIF_NONE;
unsigned long notifStartTime = 0;
const unsigned long NOTIF_DURATION = 2000;

// ============ BATTERY FUNCTION ============
float readBatteryPercentage() {
  int rawValue = analogRead(BATTERY_PIN);
  float voltageAtPin = (rawValue / 4095.0) * 3.3;
  
  // Hitung tegangan baterai sebenarnya
  float batteryVoltage = voltageAtPin * (R1 + R2) / R2;
  
  // Hitung persentase (linear antara 3.0V - 4.2V)
  float percent = (batteryVoltage - BATT_EMPTY_VOLTAGE) / 
                  (BATT_FULL_VOLTAGE - BATT_EMPTY_VOLTAGE) * 100;
  
  // Batasi range 0-100%
  percent = constrain(percent, 0, 100);
  
  // Debug via Serial
  Serial.print("ADC: ");
  Serial.print(rawValue);
  Serial.print(" | Pin: ");
  Serial.print(voltageAtPin, 2);
  Serial.print("V | Baterai: ");
  Serial.print(batteryVoltage, 2);
  Serial.print("V | ");
  Serial.print(percent, 1);
  Serial.println("%");
  
  return percent;
}

// ============ GAMBAR IKON BATTERY ============
void drawBatteryIcon(int x, int y, float percent) {
  // Badan baterai
  display.drawRect(x, y, 20, 10, SSD1306_WHITE);
  // Terminal
  display.fillRect(x + 20, y + 2, 2, 6, SSD1306_WHITE);
  
  // Isi baterai
  int fillWidth = map(percent, 0, 100, 0, 18);
  if (fillWidth > 0) {
    display.fillRect(x + 1, y + 1, fillWidth, 8, SSD1306_WHITE);
  }
  
  // Tampilkan persentase
  display.setCursor(x + 25, y + 1);
  display.setTextSize(1);
  display.print((int)percent);
  display.print("%");
  
  // Warning jika baterai low (<15%)
  if (percent < 15) {
    display.setCursor(x + 40, y + 1);
    display.print("!");
  }
}

// ============ GAMBAR IKON BLUETOOTH ============
void drawBluetoothIcon(int x, int y, bool connected) {
  if (connected) {
    display.fillTriangle(x, y + 3, x + 8, y + 6, x, y + 9, SSD1306_WHITE);
    display.fillTriangle(x, y, x + 8, y + 6, x, y + 12, SSD1306_WHITE);
    display.fillCircle(x + 4, y + 6, 2, SSD1306_BLACK);
  } else {
    display.drawTriangle(x, y + 3, x + 8, y + 6, x, y + 9, SSD1306_WHITE);
    display.drawTriangle(x, y, x + 8, y + 6, x, y + 12, SSD1306_WHITE);
    display.drawLine(x - 2, y - 2, x + 10, y + 14, SSD1306_WHITE);
    display.drawLine(x + 10, y - 2, x - 2, y + 14, SSD1306_WHITE);
  }
}

// ============ TAMPILAN NOTIFIKASI ============
void showNotification(NotificationType notif) {
  currentNotif = notif;
  notifStartTime = millis();
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  switch(notif) {
    case NOTIF_SEND_SUCCESS:
      display.setCursor(20, 25);
      display.println("PENGIRIMAN");
      display.setCursor(25, 40);
      display.println("BERHASIL!");
      break;
    case NOTIF_SEND_FAILED:
      display.setCursor(20, 25);
      display.println("PENGIRIMAN");
      display.setCursor(25, 40);
      display.println("GAGAL!");
      break;
    default:
      break;
  }
  display.display();
}

// ============ TAMPILAN UTAMA ============
void displayMainScreen() {
  display.clearDisplay();
  
  // Header: Bluetooth (kiri) dan Baterai (kanan)
  drawBluetoothIcon(2, 0, bluetoothConnected);
  drawBatteryIcon(SCREEN_WIDTH - 55, 0, batteryPercent);
  
  // Garis pemisah
  display.drawLine(0, 13, SCREEN_WIDTH, 13, SSD1306_WHITE);
  
  // Label BERAT
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("BERAT:");
  
  // Nilai berat
  display.setTextSize(3);
  display.setCursor(15, 40);
  
  if (isnan(lastWeight) || lastWeight == 0.0) {
    display.println("0.00");
  } else {
    display.println(String(lastWeight, 2));
  }
  
  // Satuan kg
  display.setTextSize(1);
  display.setCursor(95, 55);
  display.println("kg");
  
  // Info siap terima perintah
  display.setTextSize(1);
  display.setCursor(2, 56);
  display.println("GET_WEIGHT ready");
  
  display.display();
}

// ============ UPDATE DISPLAY ============
void updateDisplay() {
  static unsigned long lastBatteryRead = 0;
  
  if (millis() - lastBatteryRead > 5000) {
    lastBatteryRead = millis();
    batteryPercent = readBatteryPercentage();
  }
  
  if (currentNotif != NOTIF_NONE) {
    if (millis() - notifStartTime > NOTIF_DURATION) {
      currentNotif = NOTIF_NONE;
      displayMainScreen();
    }
  } else {
    displayMainScreen();
  }
}

// ============ PARSING BERAT ============
String extractNumericPart(String inputString) {
    String numericPart = "";
    bool foundNumeric = false;
    bool foundDecimal = false;
    bool foundNegative = false;

    inputString.trim();

    for (int i = 0; i < inputString.length(); i++) {
        char currentChar = inputString.charAt(i);

        if (isdigit(currentChar)) {
            numericPart += currentChar;
            foundNumeric = true;
        } else if (currentChar == '.' && !foundDecimal && foundNumeric) {
            numericPart += currentChar;
            foundDecimal = true;
        } else if (currentChar == '-' && !foundNumeric && !foundNegative) {
            numericPart += currentChar;
            foundNegative = true;
        } else if (foundNumeric) {
            break;
        }
    }

    return numericPart;
}

// ============ BACA TIMBANGAN ============
float readWeight() {
    static String buffer = "";

    while (rs232Serial.available()) {
        char c = rs232Serial.read();

        if (c == '\n') {
            String numericPart = extractNumericPart(buffer);
            buffer = "";

            if (numericPart.length() > 0) {
                lastWeight = numericPart.toFloat();
                Serial.println("UPDATE: " + String(lastWeight));
            }
        } 
        else if (buffer.length() < 30) {
            buffer += c;
        } 
        else {
            buffer = "";
        }
    }

    return lastWeight;
}

// ============ STATUS BLUETOOTH ============
void setBluetoothStatus(bool connected) {
    bluetoothConnected = connected;
    
    if (connected) {
        digitalWrite(LED_MERAH, LOW);
        digitalWrite(LED_HIJAU, HIGH);
        Serial.println("Bluetooth: TERHUBUNG - LED Hijau ON");
    } else {
        digitalWrite(LED_HIJAU, LOW);
        digitalWrite(LED_MERAH, HIGH);
        Serial.println("Bluetooth: MENUNGGU - LED Merah ON");
    }
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    
    pinMode(LED_HIJAU, OUTPUT);
    pinMode(LED_MERAH, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    
    setBluetoothStatus(false);
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("OLED ERROR!");
    }
    display.clearDisplay();
    display.display();
    
    rs232Serial.begin(9600, SERIAL_8N1, WEIGHT_RX_PIN, -1);
    SerialBT.begin("ScaleReader");
    
    if (SerialBT.hasClient()) {
        setBluetoothStatus(true);
    }
    
    batteryPercent = readBatteryPercentage();
    displayMainScreen();
    
    Serial.println("Ready bro, kirim GET_WEIGHT");
    Serial.println("Baterai 15000mAH 3.7V terdeteksi");
}

// ============ LOOP ============
void loop() {
    readWeight();
    
    static bool lastConnectionStatus = false;
    bool currentConnectionStatus = SerialBT.hasClient();
    
    if (currentConnectionStatus != lastConnectionStatus) {
        setBluetoothStatus(currentConnectionStatus);
        lastConnectionStatus = currentConnectionStatus;
        updateDisplay();
    }
    
    while (SerialBT.available()) {
        char c = SerialBT.read();

        if (c == '\n') {
            btCommand.trim();
            Serial.println("CMD: " + btCommand);

            if (btCommand == "GET_WEIGHT") {
                String response;
                bool sendSuccess = false;

                if (isnan(lastWeight) || lastWeight == 0.0) {
                    response = "NO_DATA";
                } else {
                    response = String(lastWeight, 2) + " kg";
                }

                for (int i = 0; i < 3; i++) {
                    SerialBT.println(response);
                    delay(10);
                    sendSuccess = true;
                    break;
                }

                if (sendSuccess) {
                    Serial.println("SEND: " + response);
                    showNotification(NOTIF_SEND_SUCCESS);
                } else {
                    Serial.println("SEND FAILED: " + response);
                    showNotification(NOTIF_SEND_FAILED);
                }
            }

            btCommand = "";
        } else {
            btCommand += c;
        }
    }
    
    updateDisplay();
    delay(10);
}