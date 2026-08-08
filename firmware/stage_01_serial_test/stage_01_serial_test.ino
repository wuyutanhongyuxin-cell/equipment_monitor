constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr unsigned long PRINT_INTERVAL_MS = 1000;

unsigned long counter = 0;
unsigned long lastPrintMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  Serial.println("ESP32 monitor boot OK");
}

void loop() {
  const unsigned long now = millis();

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    counter++;
    Serial.print("counter: ");
    Serial.println(counter);
  }
}

