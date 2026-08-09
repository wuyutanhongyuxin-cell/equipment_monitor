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
constexpr int STATUS_LED_PIN = 2;
constexpr uint32_t I2C_FREQUENCY_HZ = 50000;
constexpr uint32_t SAMPLE_RATE_HZ = 200;
constexpr uint32_t SAMPLE_INTERVAL_US = 1000000UL / SAMPLE_RATE_HZ;
constexpr uint32_t WINDOW_SAMPLES = SAMPLE_RATE_HZ;
constexpr uint32_t CALIBRATION_SAMPLES = SAMPLE_RATE_HZ * 2;
constexpr uint32_t STILL_WINDOWS = 15;
constexpr uint32_t PREPARE_WINDOWS = 10;
constexpr uint32_t VIBRATION_WINDOWS = 15;
constexpr float ACCEL_LSB_PER_G = 16384.0F;
constexpr float GRAVITY_FILTER_ALPHA = 0.015466F;

enum class Phase { WAITING, STILL, PREPARE, VIBRATION, DONE };

struct Acceleration { float x; float y; float z; };
struct DatasetSummary {
  float rmsMin = 1000.0F;
  float rmsMax = 0.0F;
  float rmsSum = 0.0F;
  float peakMax = 0.0F;
  uint32_t windows = 0;
};

Acceleration gravity = {0.0F, 0.0F, 0.0F};
DatasetSummary stillSummary;
DatasetSummary vibrationSummary;
Phase phase = Phase::WAITING;
uint32_t phaseWindow = 0;
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

bool readRegisters(uint8_t reg, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return false;
  if (Wire.requestFrom(MPU6050_ADDRESS, length, true) != length) return false;
  for (size_t i = 0; i < length; ++i) buffer[i] = Wire.read();
  return true;
}

int16_t combineBytes(uint8_t highByte, uint8_t lowByte) {
  return static_cast<int16_t>((static_cast<uint16_t>(highByte) << 8) | lowByte);
}

bool readAcceleration(Acceleration &accel) {
  uint8_t data[6];
  if (!readRegisters(REG_ACCEL_XOUT_H, data, sizeof(data))) return false;
  accel.x = combineBytes(data[0], data[1]) / ACCEL_LSB_PER_G;
  accel.y = combineBytes(data[2], data[3]) / ACCEL_LSB_PER_G;
  accel.z = combineBytes(data[4], data[5]) / ACCEL_LSB_PER_G;
  return true;
}

void stopWithError(const char *message) {
  Serial.println(message);
  while (true) delay(1000);
}

bool readExpectedRegister(uint8_t reg, uint8_t expected) {
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    uint8_t value = 0;
    if (readRegisters(reg, &value, 1) && value == expected) return true;
    delay(20);
  }
  return false;
}

void verifyIdentity() {
  uint8_t identity = 0;
  if (!readRegisters(REG_WHO_AM_I, &identity, 1) ||
      (identity != 0x68 && identity != 0x74)) {
    stopWithError("Sensor init: unsupported identity");
  }
  Serial.printf("Sensor init: WHO_AM_I=0x%02X%s\n", identity,
                identity == 0x74 ? ", non-standard compatible device" : "");
}

void calibrateGravity() {
  Serial.printf("Calibration: keep sensor stationary, samples=%lu\n", CALIBRATION_SAMPLES);
  uint32_t valid = 0;
  uint32_t failures = 0;
  uint32_t dueUs = micros();
  while (valid < CALIBRATION_SAMPLES) {
    while (static_cast<int32_t>(micros() - dueUs) < 0) delayMicroseconds(100);
    dueUs += SAMPLE_INTERVAL_US;
    Acceleration accel;
    if (!readAcceleration(accel)) {
      if (++failures > 10) stopWithError("Calibration: too many I2C failures");
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

void resetWindow(uint32_t nowUs) {
  windowStartUs = nowUs;
  windowAttempts = 0;
  windowValid = 0;
  windowFailures = 0;
  windowMissed = 0;
  windowSumSquares = 0.0F;
  windowPeakG = 0.0F;
}

void updateSummary(DatasetSummary &summary, float rmsG, float peakG) {
  if (rmsG < summary.rmsMin) summary.rmsMin = rmsG;
  if (rmsG > summary.rmsMax) summary.rmsMax = rmsG;
  if (peakG > summary.peakMax) summary.peakMax = peakG;
  summary.rmsSum += rmsG;
  ++summary.windows;
}

void printSummary() {
  const float stillMean = stillSummary.rmsSum / stillSummary.windows;
  const float vibrationMean = vibrationSummary.rmsSum / vibrationSummary.windows;
  Serial.printf("summary,label=STILL,windows=%lu,rms_min=%.5f,rms_mean=%.5f,rms_max=%.5f,peak_max=%.5f\n",
                stillSummary.windows, stillSummary.rmsMin, stillMean,
                stillSummary.rmsMax, stillSummary.peakMax);
  Serial.printf("summary,label=VIBRATION,windows=%lu,rms_min=%.5f,rms_mean=%.5f,rms_max=%.5f,peak_max=%.5f\n",
                vibrationSummary.windows, vibrationSummary.rmsMin, vibrationMean,
                vibrationSummary.rmsMax, vibrationSummary.peakMax);
  if (vibrationSummary.rmsMin > stillSummary.rmsMax) {
    const float candidate = (vibrationSummary.rmsMin + stillSummary.rmsMax) * 0.5F;
    Serial.printf("threshold_candidate: vibration_rms_g=%.5f, separation_gap_g=%.5f\n",
                  candidate, vibrationSummary.rmsMin - stillSummary.rmsMax);
  } else {
    Serial.println("threshold_candidate: none, datasets overlap");
  }
  Serial.println("Dataset: complete; send g to repeat after keeping sensor stationary");
}

void finishWindow(uint32_t nowUs) {
  const float elapsedSeconds = (nowUs - windowStartUs) / 1000000.0F;
  const float rateHz = windowValid / elapsedSeconds;
  const float rmsG = windowValid ? sqrtf(windowSumSquares / windowValid) : 0.0F;

  if (phase == Phase::STILL || phase == Phase::VIBRATION) {
    const char *label = phase == Phase::STILL ? "STILL" : "VIBRATION";
    Serial.printf("data,label=%s,index=%lu,rate_hz=%.2f,valid=%lu/%lu,failed=%lu,missed=%lu,rms_g=%.5f,peak_g=%.5f\n",
                  label, phaseWindow + 1, rateHz, windowValid, windowAttempts,
                  windowFailures, windowMissed, rmsG, windowPeakG);
    updateSummary(phase == Phase::STILL ? stillSummary : vibrationSummary,
                  rmsG, windowPeakG);
  } else if (phase == Phase::PREPARE) {
    Serial.printf("Prepare vibration: %lu seconds remaining\n", PREPARE_WINDOWS - phaseWindow);
  }

  ++phaseWindow;
  if (phase == Phase::STILL && phaseWindow >= STILL_WINDOWS) {
    phase = Phase::PREPARE;
    phaseWindow = 0;
    Serial.println("Phase: PREPARE; start repeated gentle taps when LED turns on");
  } else if (phase == Phase::PREPARE && phaseWindow >= PREPARE_WINDOWS) {
    phase = Phase::VIBRATION;
    phaseWindow = 0;
    digitalWrite(STATUS_LED_PIN, HIGH);
    Serial.println("Phase: VIBRATION; LED ON; tap or gently move continuously");
  } else if (phase == Phase::VIBRATION && phaseWindow >= VIBRATION_WINDOWS) {
    phase = Phase::DONE;
    digitalWrite(STATUS_LED_PIN, LOW);
    Serial.println("Phase: DONE; LED OFF; stop moving sensor");
    printSummary();
  }
  resetWindow(nowUs);
}

void startDataset() {
  stillSummary = DatasetSummary();
  vibrationSummary = DatasetSummary();
  phase = Phase::STILL;
  phaseWindow = 0;
  digitalWrite(STATUS_LED_PIN, LOW);
  nextSampleUs = micros() + SAMPLE_INTERVAL_US;
  resetWindow(micros());
  Serial.println("Phase: STILL; do not touch sensor for 15 seconds");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("ESP32 threshold dataset test boot OK");
  Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY_HZ);
  Wire.setTimeOut(50);
  delay(20);
  verifyIdentity();
  if (!writeRegister(REG_PWR_MGMT_1, 0x01) || !writeRegister(REG_CONFIG, 0x03) ||
      !writeRegister(REG_SMPLRT_DIV, 0x04) || !writeRegister(REG_ACCEL_CONFIG, 0x00)) {
    stopWithError("Sensor init: configuration failed");
  }
  delay(20);
  if (!readExpectedRegister(REG_CONFIG, 0x03) ||
      !readExpectedRegister(REG_SMPLRT_DIV, 0x04) ||
      !readExpectedRegister(REG_ACCEL_CONFIG, 0x00)) {
    stopWithError("Sensor init: configuration verification failed");
  }
  calibrateGravity();
  Serial.println("Dataset: ready; send g to start");
}

void loop() {
  if ((phase == Phase::WAITING || phase == Phase::DONE) && Serial.available()) {
    if (Serial.read() == 'g') startDataset();
  }
  if (phase == Phase::WAITING || phase == Phase::DONE) return;

  const uint32_t nowUs = micros();
  if (static_cast<int32_t>(nowUs - nextSampleUs) < 0) return;
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
    const float dx = accel.x - gravity.x;
    const float dy = accel.y - gravity.y;
    const float dz = accel.z - gravity.z;
    const float squared = dx * dx + dy * dy + dz * dz;
    const float magnitude = sqrtf(squared);
    windowSumSquares += squared;
    if (magnitude > windowPeakG) windowPeakG = magnitude;
    ++windowValid;
  }
  if (windowAttempts >= WINDOW_SAMPLES) finishWindow(micros());
}
