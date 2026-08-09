#include <Wire.h>

constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr uint8_t REG_SMPLRT_DIV = 0x19;
constexpr uint8_t REG_CONFIG = 0x1A;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_WHO_AM_I = 0x75;

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 50000;
constexpr uint32_t SAMPLE_RATE_HZ = 200;
constexpr uint32_t SAMPLE_INTERVAL_US = 1000000UL / SAMPLE_RATE_HZ;
constexpr uint32_t WINDOW_SAMPLES = SAMPLE_RATE_HZ;
constexpr uint32_t CALIBRATION_SAMPLES = SAMPLE_RATE_HZ * 2;
constexpr uint8_t INIT_MAX_ATTEMPTS = 5;
constexpr float ACCEL_LSB_PER_G = 16384.0F;
constexpr float GRAVITY_FILTER_ALPHA = 0.015466F;  // 0.5 Hz low-pass at 200 Hz.

struct Acceleration {
  float x;
  float y;
  float z;
};

Acceleration gravity = {0.0F, 0.0F, 0.0F};
uint32_t nextSampleUs = 0;
uint32_t windowStartUs = 0;
uint32_t windowAttempts = 0;
uint32_t windowValid = 0;
uint32_t windowFailures = 0;
uint32_t windowMissed = 0;
float windowSumSquares = 0.0F;
float windowPeakG = 0.0F;

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t startRegister, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(startRegister);
  if (Wire.endTransmission(true) != 0) {
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

bool readAcceleration(Acceleration &accel) {
  uint8_t data[6];
  if (!readRegisters(REG_ACCEL_XOUT_H, data, sizeof(data))) {
    return false;
  }

  accel.x = combineBytes(data[0], data[1]) / ACCEL_LSB_PER_G;
  accel.y = combineBytes(data[2], data[3]) / ACCEL_LSB_PER_G;
  accel.z = combineBytes(data[4], data[5]) / ACCEL_LSB_PER_G;
  return true;
}

bool readExpectedRegister(uint8_t reg, uint8_t expected, const char *name) {
  for (uint8_t attempt = 1; attempt <= INIT_MAX_ATTEMPTS; ++attempt) {
    uint8_t value = 0;
    if (readRegisters(reg, &value, 1) && value == expected) {
      if (attempt > 1) {
        Serial.printf("MPU6050 init: %s verified on attempt %u\n", name, attempt);
      }
      return true;
    }

    Serial.printf("MPU6050 init: %s attempt %u rejected, value=0x%02X\n",
                  name, attempt, value);
    delay(20);
  }
  return false;
}

bool verifyIdentity() {
  for (uint8_t attempt = 1; attempt <= INIT_MAX_ATTEMPTS; ++attempt) {
    uint8_t value = 0;
    if (readRegisters(REG_WHO_AM_I, &value, 1)) {
      if (value == 0x68) {
        Serial.println("MPU6050 init: WHO_AM_I=0x68");
        return true;
      }
      if (value == 0x74) {
        Serial.println("MPU6050 init: WHO_AM_I=0x74, non-standard compatible device");
        return true;
      }
    }

    Serial.printf("MPU6050 init: WHO_AM_I attempt %u rejected, value=0x%02X\n",
                  attempt, value);
    delay(20);
  }
  return false;
}

void stopWithError(const char *message) {
  Serial.println(message);
  while (true) {
    delay(1000);
  }
}

void calibrateGravity() {
  Serial.printf("Calibration: keep sensor stationary, samples=%lu\n", CALIBRATION_SAMPLES);
  uint32_t valid = 0;
  uint32_t failures = 0;
  uint32_t calibrationNextUs = micros();

  while (valid < CALIBRATION_SAMPLES) {
    while (static_cast<int32_t>(micros() - calibrationNextUs) < 0) {
      delayMicroseconds(100);
    }
    calibrationNextUs += SAMPLE_INTERVAL_US;

    Acceleration accel;
    if (!readAcceleration(accel)) {
      ++failures;
      if (failures > 10) {
        stopWithError("Calibration: too many I2C read failures");
      }
      continue;
    }

    gravity.x += accel.x;
    gravity.y += accel.y;
    gravity.z += accel.z;
    ++valid;
  }

  gravity.x /= valid;
  gravity.y /= valid;
  gravity.z /= valid;
  Serial.printf("Calibration: ready, gravity_g=(%+.4f,%+.4f,%+.4f), failures=%lu\n",
                gravity.x, gravity.y, gravity.z, failures);
}

void printAndResetWindow(uint32_t nowUs) {
  const float elapsedSeconds = (nowUs - windowStartUs) / 1000000.0F;
  const float actualRateHz = windowValid / elapsedSeconds;
  const float rmsG = windowValid > 0 ? sqrtf(windowSumSquares / windowValid) : 0.0F;

  Serial.printf("window: rate_hz=%.2f, valid=%lu/%lu, failed=%lu, missed=%lu, vibration_rms_g=%.5f, peak_g=%.5f\n",
                actualRateHz, windowValid, windowAttempts, windowFailures,
                windowMissed, rmsG, windowPeakG);

  windowStartUs = nowUs;
  windowAttempts = 0;
  windowValid = 0;
  windowFailures = 0;
  windowMissed = 0;
  windowSumSquares = 0.0F;
  windowPeakG = 0.0F;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 vibration sampling test boot OK");

  Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY_HZ);
  Wire.setTimeOut(50);
  delay(20);

  if (!verifyIdentity()) {
    stopWithError("MPU6050 init: identity check failed");
  }

  if (!writeRegister(REG_PWR_MGMT_1, 0x01) ||
      !writeRegister(REG_CONFIG, 0x03) ||
      !writeRegister(REG_SMPLRT_DIV, 0x04) ||
      !writeRegister(REG_ACCEL_CONFIG, 0x00)) {
    stopWithError("MPU6050 init: configuration failed");
  }

  delay(20);
  if (!readExpectedRegister(REG_CONFIG, 0x03, "CONFIG") ||
      !readExpectedRegister(REG_SMPLRT_DIV, 0x04, "SMPLRT_DIV") ||
      !readExpectedRegister(REG_ACCEL_CONFIG, 0x00, "ACCEL_CONFIG")) {
    stopWithError("MPU6050 init: configuration verification failed");
  }

  Serial.println("MPU6050 init: ready, sample_target_hz=200, dlpf_hz=44, accel_range=+/-2g");
  calibrateGravity();

  nextSampleUs = micros() + SAMPLE_INTERVAL_US;
  windowStartUs = micros();
}

void loop() {
  const uint32_t nowUs = micros();
  if (static_cast<int32_t>(nowUs - nextSampleUs) < 0) {
    return;
  }

  const uint32_t latenessUs = nowUs - nextSampleUs;
  if (latenessUs >= SAMPLE_INTERVAL_US) {
    const uint32_t skipped = latenessUs / SAMPLE_INTERVAL_US;
    windowMissed += skipped;
    nextSampleUs += skipped * SAMPLE_INTERVAL_US;
  }
  nextSampleUs += SAMPLE_INTERVAL_US;
  ++windowAttempts;

  Acceleration accel;
  if (!readAcceleration(accel)) {
    ++windowFailures;
  } else {
    gravity.x += GRAVITY_FILTER_ALPHA * (accel.x - gravity.x);
    gravity.y += GRAVITY_FILTER_ALPHA * (accel.y - gravity.y);
    gravity.z += GRAVITY_FILTER_ALPHA * (accel.z - gravity.z);

    const float vibrationX = accel.x - gravity.x;
    const float vibrationY = accel.y - gravity.y;
    const float vibrationZ = accel.z - gravity.z;
    const float vibrationSquared = vibrationX * vibrationX + vibrationY * vibrationY +
                                   vibrationZ * vibrationZ;
    const float vibrationG = sqrtf(vibrationSquared);

    windowSumSquares += vibrationSquared;
    if (vibrationG > windowPeakG) {
      windowPeakG = vibrationG;
    }
    ++windowValid;
  }

  if (windowAttempts >= WINDOW_SAMPLES) {
    printAndResetWindow(micros());
  }
}
