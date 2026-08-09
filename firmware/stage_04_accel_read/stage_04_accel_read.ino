#include <Wire.h>

constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_WHO_AM_I = 0x75;
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr float ACCEL_LSB_PER_G = 16384.0F;

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t startRegister, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(startRegister);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const size_t received = Wire.requestFrom(MPU6050_ADDRESS, length, true);
  if (received != length) {
    return false;
  }

  for (size_t i = 0; i < length; ++i) {
    buffer[i] = Wire.read();
  }
  return true;
}

int16_t combineBytes(uint8_t highByte, uint8_t lowByte) {
  return static_cast<int16_t>((static_cast<uint16_t>(highByte) << 8) | lowByte);
}

void stopWithError(const char *message) {
  Serial.println(message);
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 MPU6050 acceleration test boot OK");

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(50);

  uint8_t whoAmI = 0;
  if (!readRegisters(REG_WHO_AM_I, &whoAmI, 1)) {
    stopWithError("MPU6050 init: WHO_AM_I read failed");
  }

  Serial.printf("MPU6050 WHO_AM_I=0x%02X\n", whoAmI);
  if (whoAmI != 0x68 && whoAmI != 0x74) {
    stopWithError("MPU6050 init: unexpected identity");
  }
  if (whoAmI == 0x74) {
    Serial.println("MPU6050 init: non-standard compatible device");
  }

  if (!writeRegister(REG_PWR_MGMT_1, 0x00)) {
    stopWithError("MPU6050 init: wake failed");
  }

  delay(100);
  Serial.println("MPU6050 init: ready, accel_range=+/-2g");
}

void loop() {
  uint8_t data[6];
  if (!readRegisters(REG_ACCEL_XOUT_H, data, sizeof(data))) {
    Serial.println("MPU6050 read: failed");
    delay(500);
    return;
  }

  const int16_t rawX = combineBytes(data[0], data[1]);
  const int16_t rawY = combineBytes(data[2], data[3]);
  const int16_t rawZ = combineBytes(data[4], data[5]);
  const float accelX = rawX / ACCEL_LSB_PER_G;
  const float accelY = rawY / ACCEL_LSB_PER_G;
  const float accelZ = rawZ / ACCEL_LSB_PER_G;
  const float magnitude = sqrtf(accelX * accelX + accelY * accelY + accelZ * accelZ);

  Serial.printf("accel_g: x=%+.3f, y=%+.3f, z=%+.3f, magnitude=%.3f\n",
                accelX, accelY, accelZ, magnitude);
  delay(500);
}
