#include <Wire.h>

#ifndef MONITOR_EARLY_SETUP
#define MONITOR_EARLY_SETUP() do {} while (0)
#endif

#ifndef MONITOR_EXTRA_SETUP
#define MONITOR_EXTRA_SETUP() do {} while (0)
#endif

#ifndef MONITOR_STATE_UPDATED
#define MONITOR_STATE_UPDATED(stateValue, rmsValue, peakValue, failedValue, missedValue) do {} while (0)
#endif

constexpr uint8_t SENSOR_ADDRESS = 0x68;
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
constexpr float ACCEL_LSB_PER_G = 16384.0F;
constexpr float GRAVITY_FILTER_ALPHA = 0.015466F;
constexpr float RUN_ENTER_RMS_G = 0.120F;
constexpr float STOP_ENTER_RMS_G = 0.060F;
constexpr uint8_t CONFIRM_WINDOWS = 2;
constexpr uint32_t SERIAL_BAUD = 115200;

enum class EquipmentState { UNKNOWN, STOP, RUN };
struct Acceleration { float x; float y; float z; };

Acceleration gravity = {0.0F, 0.0F, 0.0F};
EquipmentState state = EquipmentState::UNKNOWN;
uint8_t runConfirm = 0;
uint8_t stopConfirm = 0;
uint32_t nextSampleUs = 0;
uint32_t windowStartUs = 0;
uint32_t windowAttempts = 0;
uint32_t windowValid = 0;
uint32_t windowFailures = 0;
uint32_t windowMissed = 0;
float windowSumSquares = 0.0F;
float windowPeakG = 0.0F;

const char *stateName(EquipmentState value) {
  switch (value) {
    case EquipmentState::STOP: return "STOP";
    case EquipmentState::RUN: return "RUN";
    default: return "UNKNOWN";
  }
}

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t reg, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return false;
  if (Wire.requestFrom(SENSOR_ADDRESS, length, true) != length) return false;
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
  digitalWrite(STATUS_LED_PIN, LOW);
  while (true) delay(1000);
}

bool registerEquals(uint8_t reg, uint8_t expected) {
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    uint8_t value = 0;
    if (readRegisters(reg, &value, 1) && value == expected) return true;
    delay(20);
  }
  return false;
}

void initializeSensor() {
  uint8_t identity = 0;
  if (!readRegisters(REG_WHO_AM_I, &identity, 1) ||
      (identity != 0x68 && identity != 0x74)) {
    stopWithError("Sensor init: unsupported identity");
  }
  Serial.printf("Sensor init: WHO_AM_I=0x%02X%s\n", identity,
                identity == 0x74 ? ", non-standard compatible device" : "");
  if (!writeRegister(REG_PWR_MGMT_1, 0x01) || !writeRegister(REG_CONFIG, 0x03) ||
      !writeRegister(REG_SMPLRT_DIV, 0x04) || !writeRegister(REG_ACCEL_CONFIG, 0x00)) {
    stopWithError("Sensor init: configuration failed");
  }
  delay(20);
  if (!registerEquals(REG_CONFIG, 0x03) || !registerEquals(REG_SMPLRT_DIV, 0x04) ||
      !registerEquals(REG_ACCEL_CONFIG, 0x00)) {
    stopWithError("Sensor init: configuration verification failed");
  }
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
  Serial.printf("Calibration: ready, failures=%lu\n", failures);
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

void transitionTo(EquipmentState nextState, float rmsG) {
  if (state == nextState) return;
  Serial.printf("transition: from=%s,to=%s,rms_g=%.5f\n",
                stateName(state), stateName(nextState), rmsG);
  state = nextState;
  digitalWrite(STATUS_LED_PIN, state == EquipmentState::RUN ? HIGH : LOW);
}

void classifyWindow(float rmsG) {
  const char *evidence = "HOLD";
  uint8_t evidenceCount = 0;
  if (rmsG >= RUN_ENTER_RMS_G) {
    if (runConfirm < CONFIRM_WINDOWS) ++runConfirm;
    stopConfirm = 0;
    evidence = "RUN";
    evidenceCount = runConfirm;
    if (runConfirm >= CONFIRM_WINDOWS) transitionTo(EquipmentState::RUN, rmsG);
  } else if (rmsG <= STOP_ENTER_RMS_G) {
    if (stopConfirm < CONFIRM_WINDOWS) ++stopConfirm;
    runConfirm = 0;
    evidence = "STOP";
    evidenceCount = stopConfirm;
    if (stopConfirm >= CONFIRM_WINDOWS) transitionTo(EquipmentState::STOP, rmsG);
  } else {
    runConfirm = 0;
    stopConfirm = 0;
  }
  Serial.printf("state=%s,rms=%.5f,peak=%.5f,evidence=%s,confirm=%u/%u,valid=%lu/%lu,failed=%lu,missed=%lu\n",
                stateName(state), rmsG, windowPeakG, evidence, evidenceCount,
                CONFIRM_WINDOWS, windowValid, windowAttempts, windowFailures, windowMissed);
  MONITOR_STATE_UPDATED(stateName(state), rmsG, windowPeakG,
                        windowFailures, windowMissed);
}

void resetClassifier() {
  state = EquipmentState::UNKNOWN;
  runConfirm = 0;
  stopConfirm = 0;
  digitalWrite(STATUS_LED_PIN, LOW);
}

void runClassifierSelfTest() {
  bool passed = true;
  resetClassifier();

  classifyWindow(0.010F);
  passed &= state == EquipmentState::UNKNOWN;
  classifyWindow(0.010F);
  passed &= state == EquipmentState::STOP;
  classifyWindow(0.200F);
  passed &= state == EquipmentState::STOP;
  classifyWindow(0.090F);
  passed &= state == EquipmentState::STOP;
  classifyWindow(0.200F);
  passed &= state == EquipmentState::STOP;
  classifyWindow(0.200F);
  passed &= state == EquipmentState::RUN;
  classifyWindow(0.010F);
  passed &= state == EquipmentState::RUN;
  classifyWindow(0.010F);
  passed &= state == EquipmentState::STOP;

  Serial.printf("Classifier self-test: %s\n", passed ? "PASS" : "FAIL");
  resetClassifier();
  if (!passed) stopWithError("Classifier self-test failed");
}

void finishWindow(uint32_t nowUs) {
  const float rmsG = windowValid ? sqrtf(windowSumSquares / windowValid) : 0.0F;
  const float rateHz = windowValid / ((nowUs - windowStartUs) / 1000000.0F);
  if (windowValid == WINDOW_SAMPLES && windowFailures == 0 && windowMissed == 0) {
    classifyWindow(rmsG);
  } else {
    runConfirm = 0;
    stopConfirm = 0;
    Serial.println("classifier: invalid window, evidence reset");
  }
  Serial.printf("rate_hz=%.2f\n", rateHz);
  const uint32_t afterLogUs = micros();
  nextSampleUs = afterLogUs + SAMPLE_INTERVAL_US;
  resetWindow(afterLogUs);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  MONITOR_EARLY_SETUP();
  Serial.println("ESP32 state classifier boot OK");
  Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY_HZ);
  Wire.setTimeOut(50);
  delay(20);
  initializeSensor();
  runClassifierSelfTest();
  calibrateGravity();
  MONITOR_EXTRA_SETUP();
  Serial.printf("Classifier: run_enter=%.3fg, stop_enter=%.3fg, confirm_windows=%u\n",
                RUN_ENTER_RMS_G, STOP_ENTER_RMS_G, CONFIRM_WINDOWS);
  nextSampleUs = micros() + SAMPLE_INTERVAL_US;
  resetWindow(micros());
}

void loop() {
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
