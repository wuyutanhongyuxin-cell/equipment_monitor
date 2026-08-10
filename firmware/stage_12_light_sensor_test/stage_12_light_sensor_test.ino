#include <Arduino.h>

namespace {
constexpr uint8_t LIGHT_SENSOR_PIN = 34;
constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t REPORT_INTERVAL_MS = 200;
constexpr uint8_t SAMPLES_PER_REPORT = 16;

uint32_t lastReportMs = 0;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);

  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_SENSOR_PIN, ADC_11db);

  Serial.println("Stage 12 light sensor test boot OK");
  Serial.println("Input: AO -> GPIO34 (ADC1), 3.3V power only");
}

void loop() {
  const uint32_t now = millis();
  if (now - lastReportMs < REPORT_INTERVAL_MS) {
    return;
  }
  lastReportMs = now;

  uint32_t rawSum = 0;
  uint32_t millivoltSum = 0;
  for (uint8_t i = 0; i < SAMPLES_PER_REPORT; ++i) {
    rawSum += analogRead(LIGHT_SENSOR_PIN);
    millivoltSum += analogReadMilliVolts(LIGHT_SENSOR_PIN);
  }

  const uint32_t rawAverage = rawSum / SAMPLES_PER_REPORT;
  const uint32_t millivoltAverage = millivoltSum / SAMPLES_PER_REPORT;
  Serial.printf("light_raw=%lu,light_mv=%lu\n",
                static_cast<unsigned long>(rawAverage),
                static_cast<unsigned long>(millivoltAverage));
}
