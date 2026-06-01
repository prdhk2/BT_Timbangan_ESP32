#include <Arduino.h>
#include "BluetoothSerial.h"
#include "config.h"

BluetoothSerial SerialBT;

String btCommand = "";
float lastWeight = 0.0;

#define LED_HIJAU   18
#define LED_MERAH   19

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

void setBluetoothStatus(bool connected) {
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

void setup() {
    Serial.begin(115200);

    pinMode(LED_HIJAU, OUTPUT);
    pinMode(LED_MERAH, OUTPUT);
    
    setBluetoothStatus(false);

    rs232Serial.begin(9600, SERIAL_8N1, WEIGHT_RX_PIN, -1);

    SerialBT.begin("ScaleReader");
    
    if (SerialBT.hasClient()) {
        setBluetoothStatus(true);
    }

    Serial.println("Ready bro, kirim GET_WEIGHT");
}

void loop() {
    readWeight();

    static bool lastConnectionStatus = false;
    bool currentConnectionStatus = SerialBT.hasClient();
    
    if (currentConnectionStatus != lastConnectionStatus) {
        setBluetoothStatus(currentConnectionStatus);
        lastConnectionStatus = currentConnectionStatus;
    }

    while (SerialBT.available()) {
        char c = SerialBT.read();

        if (c == '\n') {
            btCommand.trim();

            Serial.println("CMD: " + btCommand);

            if (btCommand == "GET_WEIGHT") {
                String response;

                if (isnan(lastWeight) || lastWeight == 0.0) {
                    response = "NO_DATA";
                } else {
                    response = String(lastWeight, 2) + " kg";
                }

                SerialBT.println(response);
                Serial.println("SEND: " + response);
            }

            btCommand = "";
        } else {
            btCommand += c;
        }
    }

    delay(10);
}