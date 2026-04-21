#include <Arduino.h>

#define PIN_COUNT 8
static const int pins[PIN_COUNT] = { 23, 19, 18, 27, 21, 22, 16, 17 };

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);
    Serial.println("\n=== pin finder ===");
    for (int i = 0; i < PIN_COUNT; i++) {
        pinMode(pins[i], OUTPUT);
        Serial.printf("Sending LOW on %d\n", pins[i]);
        digitalWrite(pins[i], LOW);
    }
}

void loop () {
    for (int i = 0; i < PIN_COUNT; i++) {
        Serial.printf("Sending HIGH on %d\n", pins[i]);
        digitalWrite(pins[i], HIGH);
        delay(2000);
        Serial.printf("Sending LOW on %d\n", pins[i]);
        digitalWrite(pins[i], LOW);
        delay(1000);
    }
}
