#include <Arduino.h>

namespace {
constexpr uint8_t BUZZER_PIN = 25;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t STARTUP_SILENCE_MS = 2000;
constexpr uint32_t BEEP_ON_MS = 200;
constexpr uint32_t BEEP_OFF_MS = 2000;
constexpr uint8_t BEEP_COUNT = 3;
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(SERIAL_BAUD_RATE);
  delay(STARTUP_SILENCE_MS);
  Serial.println("Stage 12 active buzzer test boot OK");
  Serial.println("GPIO25, active-high, three finite beeps");

  for (uint8_t count = 1; count <= BEEP_COUNT; ++count) {
    Serial.printf("buzzer_on=%u/%u\n", count, BEEP_COUNT);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BEEP_ON_MS);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.printf("buzzer_off=%u/%u\n", count, BEEP_COUNT);
    if (count < BEEP_COUNT) {
      delay(BEEP_OFF_MS);
    }
  }

  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Buzzer test complete; output locked OFF");
}

void loop() {
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
}
