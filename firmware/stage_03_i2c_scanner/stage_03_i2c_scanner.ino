#include <Wire.h>

constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 100000;
constexpr unsigned long SCAN_INTERVAL_MS = 5000;

unsigned long lastScanMs = 0;
bool busStarted = false;

constexpr int CANDIDATE_I2C_PINS[] = {
  21, 22, 19, 23, 18, 5, 17, 16, 4, 2, 15, 13, 12, 14, 27, 26, 25, 33, 32
};
constexpr size_t CANDIDATE_I2C_PIN_COUNT = sizeof(CANDIDATE_I2C_PINS) / sizeof(CANDIDATE_I2C_PINS[0]);
constexpr byte EXPECTED_MPU_ADDRESSES[] = {0x68, 0x69};

struct I2CMapping {
  const char *label;
  int sdaPin;
  int sclPin;
};

struct ScanResult {
  byte deviceCount;
  byte expectedMpuCount;
};

struct PinLevelResult {
  int pin;
  bool pulldownLevel;
  bool pullupLevel;
};

const I2CMapping PRIMARY_MAPPING = {"primary", I2C_SDA_PIN, I2C_SCL_PIN};
const I2CMapping SWAPPED_MAPPING = {"swapped", I2C_SCL_PIN, I2C_SDA_PIN};

bool isExpectedMpuAddress(byte address) {
  return address == 0x68 || address == 0x69;
}

void printHexByte(byte value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void printLevel(bool level) {
  Serial.print(level ? "HIGH" : "LOW");
}

bool readPinWithMode(int pin, int mode) {
  pinMode(pin, mode);
  delay(20);
  int highSamples = 0;

  for (int sample = 0; sample < 5; sample++) {
    if (digitalRead(pin) == HIGH) {
      highSamples++;
    }
    delay(2);
  }

  return highSamples >= 3;
}

PinLevelResult diagnosePinLevel(int pin) {
  const bool pulldownLevel = readPinWithMode(pin, INPUT_PULLDOWN);
  const bool pullupLevel = readPinWithMode(pin, INPUT_PULLUP);
  return {pin, pulldownLevel, pullupLevel};
}

void printPinLevelInterpretation(const PinLevelResult &result) {
  Serial.print("I2C line level: GPIO");
  Serial.print(result.pin);
  Serial.print(", internal pulldown=");
  printLevel(result.pulldownLevel);
  Serial.print(", internal pullup=");
  printLevel(result.pullupLevel);
  Serial.print(" -> ");

  if (result.pulldownLevel && result.pullupLevel) {
    Serial.println("external high/pullup detected");
  } else if (!result.pulldownLevel && result.pullupLevel) {
    Serial.println("no external pullup detected");
  } else if (!result.pulldownLevel && !result.pullupLevel) {
    Serial.println("line held LOW or shorted to GND");
  } else {
    Serial.println("unstable line, inspect wiring");
  }
}

void runLineLevelDiagnostic() {
  if (busStarted) {
    Wire.end();
    busStarted = false;
  }

  Serial.println("I2C line level diagnostic: expected connected idle lines are HIGH");
  printPinLevelInterpretation(diagnosePinLevel(I2C_SDA_PIN));
  printPinLevelInterpretation(diagnosePinLevel(I2C_SCL_PIN));
}

void configureI2CBus(const I2CMapping &mapping, bool verbose = true) {
  if (busStarted) {
    Wire.end();
  }

  pinMode(mapping.sdaPin, INPUT_PULLUP);
  pinMode(mapping.sclPin, INPUT_PULLUP);
  Wire.begin(mapping.sdaPin, mapping.sclPin, I2C_FREQUENCY_HZ);
  Wire.setTimeOut(50);
  busStarted = true;
  delay(20);

  if (!verbose) {
    return;
  }

  Serial.print("I2C mapping: ");
  Serial.print(mapping.label);
  Serial.print(", SDA=GPIO");
  Serial.print(mapping.sdaPin);
  Serial.print(", SCL=GPIO");
  Serial.println(mapping.sclPin);
}

ScanResult scanI2CBus(const I2CMapping &mapping) {
  byte deviceCount = 0;
  byte expectedMpuCount = 0;

  Serial.println("I2C scan: starting");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    const byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      printHexByte(address);
      Serial.println();
      if (isExpectedMpuAddress(address)) {
        expectedMpuCount++;
      }
      deviceCount++;
    } else if (error == 4) {
      Serial.print("I2C unknown error at 0x");
      printHexByte(address);
      Serial.println();
    }
  }

  if (deviceCount == 0) {
    Serial.println("I2C scan: no devices found");
  } else {
    Serial.print("I2C scan: complete, devices=");
    Serial.println(deviceCount);
  }

  if (deviceCount == 1 && expectedMpuCount == 1) {
    Serial.print("I2C diagnostic: ");
    Serial.print(mapping.label);
    Serial.println(" mapping has exactly one expected MPU6050 address");
  }

  return {deviceCount, expectedMpuCount};
}

byte probeExpectedMpuAddress() {
  for (const byte address : EXPECTED_MPU_ADDRESSES) {
    Wire.beginTransmission(address);
    const byte error = Wire.endTransmission();
    if (error == 0) {
      return address;
    }
  }

  return 0;
}

bool runWireFinder() {
  bool foundExpectedAddress = false;

  Serial.print("I2C wire finder: probing ");
  Serial.print(CANDIDATE_I2C_PIN_COUNT);
  Serial.println(" candidate GPIO pins for 0x68/0x69");

  for (size_t sdaIndex = 0; sdaIndex < CANDIDATE_I2C_PIN_COUNT; sdaIndex++) {
    for (size_t sclIndex = 0; sclIndex < CANDIDATE_I2C_PIN_COUNT; sclIndex++) {
      const int sdaPin = CANDIDATE_I2C_PINS[sdaIndex];
      const int sclPin = CANDIDATE_I2C_PINS[sclIndex];

      if (sdaPin == sclPin) {
        continue;
      }

      const I2CMapping candidateMapping = {"wire-finder", sdaPin, sclPin};
      configureI2CBus(candidateMapping, false);
      const byte foundAddress = probeExpectedMpuAddress();

      if (foundAddress != 0) {
        foundExpectedAddress = true;
        Serial.print("I2C wire finder: found expected address 0x");
        printHexByte(foundAddress);
        Serial.print(" with SDA=GPIO");
        Serial.print(sdaPin);
        Serial.print(", SCL=GPIO");
        Serial.println(sclPin);
      }
    }
  }

  if (foundExpectedAddress) {
    Serial.println("I2C wire finder: rewire to primary GPIO21/GPIO22 before Stage 3 can pass");
  } else {
    Serial.println("I2C wire finder: no expected address found on candidate GPIO pairs");
  }

  return foundExpectedAddress;
}

void runDiagnosticScan() {
  configureI2CBus(PRIMARY_MAPPING);
  const ScanResult primaryResult = scanI2CBus(PRIMARY_MAPPING);

  if (primaryResult.deviceCount == 1 && primaryResult.expectedMpuCount == 1) {
    Serial.println("I2C diagnostic: Stage 3 pass candidate on primary wiring");
    return;
  }

  configureI2CBus(SWAPPED_MAPPING);
  const ScanResult swappedResult = scanI2CBus(SWAPPED_MAPPING);

  if (swappedResult.expectedMpuCount > 0) {
    Serial.println("I2C diagnostic: MPU6050 responds only with SDA/SCL swapped in software");
    Serial.println("I2C diagnostic: power off and swap the P21/P22 wires before retesting");
  } else if (primaryResult.deviceCount == 0 && swappedResult.deviceCount == 0) {
    Serial.println("I2C diagnostic: no devices on either mapping");
    Serial.println("I2C diagnostic: check 3V3, GND, breadboard rows, and jumper contact");
    runLineLevelDiagnostic();
    runWireFinder();
  } else {
    Serial.println("I2C diagnostic: unexpected I2C device result, investigate before Stage 4");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  Serial.println("ESP32 I2C diagnostic scanner boot OK");
  Serial.println("I2C primary wiring: SDA=GPIO21, SCL=GPIO22, frequency=100000 Hz");

  runDiagnosticScan();
  lastScanMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = now;
    runDiagnosticScan();
  }

  delay(10);
}
