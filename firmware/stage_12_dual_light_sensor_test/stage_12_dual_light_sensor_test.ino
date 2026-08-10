#include <Arduino.h>

namespace {
constexpr uint8_t SENSOR_1_PIN = 34;
constexpr uint8_t SENSOR_2_PIN = 35;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t REPORT_INTERVAL_MS = 200;
constexpr uint8_t SAMPLES_PER_REPORT = 16;

uint32_t lastReportMs = 0;

uint32_t readAveragedRaw(uint8_t pin) {
  analogRead(pin);  // Discard the first conversion after switching ADC channels.
  uint32_t sum = 0;
  for (uint8_t i = 0; i < SAMPLES_PER_REPORT; ++i) {
    sum += analogRead(pin);
  }
  return sum / SAMPLES_PER_REPORT;
}
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR_1_PIN, ADC_11db);
  analogSetPinAttenuation(SENSOR_2_PIN, ADC_11db);

  Serial.println("Stage 12 dual light sensor test boot OK");
  Serial.println("S1 AO -> GPIO34, S2 AO -> GPIO35, 3.3V power only");
}

void loop() {
  const uint32_t now = millis();
  if (now - lastReportMs < REPORT_INTERVAL_MS) {
    return;
  }
  lastReportMs = now;

  const uint32_t sensor1Raw = readAveragedRaw(SENSOR_1_PIN);
  const uint32_t sensor2Raw = readAveragedRaw(SENSOR_2_PIN);
  Serial.printf("s1_raw=%lu,s2_raw=%lu\n",
                static_cast<unsigned long>(sensor1Raw),
                static_cast<unsigned long>(sensor2Raw));
}
