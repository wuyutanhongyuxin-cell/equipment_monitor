#include <Wire.h>

constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 100000;
constexpr unsigned long SCAN_INTERVAL_MS = 5000;

unsigned long lastScanMs = 0;

void scanI2CBus() {
  byte deviceCount = 0;

  Serial.println("I2C scan: starting");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    const byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 0x10) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
      deviceCount++;
    } else if (error == 4) {
      Serial.print("I2C unknown error at 0x");
      if (address < 0x10) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("I2C scan: no devices found");
  } else {
    Serial.print("I2C scan: complete, devices=");
    Serial.println(deviceCount);
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  Serial.println("ESP32 I2C scanner boot OK");
  Serial.println("I2C pins: SDA=GPIO21, SCL=GPIO22, frequency=100000 Hz");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQUENCY_HZ);
  Wire.setTimeOut(50);
  scanI2CBus();
  lastScanMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = now;
    scanI2CBus();
  }

  delay(10);
}

