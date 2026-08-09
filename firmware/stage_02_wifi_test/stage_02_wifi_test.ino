#include <WiFi.h>

#include "secrets.h"

constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
constexpr unsigned long STATUS_INTERVAL_MS = 5000;

unsigned long lastConnectAttemptMs = 0;
unsigned long lastStatusMs = 0;
bool wasConnected = false;

void startWiFiConnection(unsigned long now) {
  Serial.print("WiFi: connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastConnectAttemptMs = now;
}

void printWiFiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi: connected, IP=");
    Serial.print(WiFi.localIP());
    Serial.print(", RSSI=");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.print("WiFi: disconnected, status=");
    Serial.println(static_cast<int>(WiFi.status()));
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  Serial.println("ESP32 WiFi test boot OK");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  startWiFiConnection(millis());
}

void loop() {
  const unsigned long now = millis();
  const bool isConnected = WiFi.status() == WL_CONNECTED;

  if (isConnected != wasConnected) {
    wasConnected = isConnected;
    if (isConnected) {
      Serial.println("WiFi: connection established");
      printWiFiStatus();
    } else {
      Serial.println("WiFi: connection lost");
    }
  }

  if (!isConnected && now - lastConnectAttemptMs >= WIFI_RETRY_INTERVAL_MS) {
    WiFi.disconnect();
    startWiFiConnection(now);
  }

  if (now - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = now;
    printWiFiStatus();
  }

  // The loop remains available for sensor monitoring during WiFi outages.
  delay(10);
}
