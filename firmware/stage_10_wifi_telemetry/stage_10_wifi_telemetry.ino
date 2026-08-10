#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#elif __has_include("../stage_02_wifi_test/secrets.h")
#include "../stage_02_wifi_test/secrets.h"
#else
#error "Create secrets.h from secrets.example.h"
#endif

void telemetrySetup();
void telemetryStateUpdated(const char *stateValue, float rmsValue, float peakValue,
                           uint32_t failedValue, uint32_t missedValue);

#define MONITOR_EXTRA_SETUP() telemetrySetup()
#define MONITOR_STATE_UPDATED(stateValue, rmsValue, peakValue, failedValue, missedValue) \
  telemetryStateUpdated(stateValue, rmsValue, peakValue, failedValue, missedValue)

#include "../stage_07_state_classifier/stage_07_state_classifier.ino"

constexpr uint16_t HTTP_PORT = 80;
constexpr uint32_t WIFI_RETRY_MS = 10000;
constexpr uint32_t CLIENT_WAIT_MS = 50;

#ifndef WIFI_RECONNECT_SELF_TEST_MS
#define WIFI_RECONNECT_SELF_TEST_MS 0
#endif

struct TelemetrySnapshot {
  char state[8];
  float rmsG;
  float peakG;
  uint32_t failedSamples;
  uint32_t missedPeriods;
  uint32_t sequence;
  uint32_t updatedMs;
};

TelemetrySnapshot telemetrySnapshot = {
  "UNKNOWN", 0.0F, 0.0F, 0, 0, 0, 0
};
portMUX_TYPE telemetryMux = portMUX_INITIALIZER_UNLOCKED;
WiFiServer statusServer(HTTP_PORT);

void copySnapshot(TelemetrySnapshot &destination) {
  portENTER_CRITICAL(&telemetryMux);
  destination = telemetrySnapshot;
  portEXIT_CRITICAL(&telemetryMux);
}

void telemetryStateUpdated(const char *stateValue, float rmsValue, float peakValue,
                           uint32_t failedValue, uint32_t missedValue) {
  portENTER_CRITICAL(&telemetryMux);
  snprintf(telemetrySnapshot.state, sizeof(telemetrySnapshot.state), "%s", stateValue);
  telemetrySnapshot.rmsG = rmsValue;
  telemetrySnapshot.peakG = peakValue;
  telemetrySnapshot.failedSamples = failedValue;
  telemetrySnapshot.missedPeriods = missedValue;
  ++telemetrySnapshot.sequence;
  telemetrySnapshot.updatedMs = millis();
  portEXIT_CRITICAL(&telemetryMux);
}

void sendStatus(WiFiClient &client) {
  TelemetrySnapshot snapshot;
  copySnapshot(snapshot);
  const uint32_t ageMs = millis() - snapshot.updatedMs;
  const bool sensorHealthy = snapshot.failedSamples == 0 && snapshot.missedPeriods == 0 &&
                             ageMs < 3000;

  char body[512];
  const int bodyLength = snprintf(
      body, sizeof(body),
      "{\"device\":\"equipment-monitor-01\",\"state\":\"%s\","
      "\"vibration_rms_g\":%.5f,\"peak_g\":%.5f,\"sequence\":%lu,"
      "\"sample_failed\":%lu,\"sample_missed\":%lu,\"sample_age_ms\":%lu,"
      "\"sensor_healthy\":%s,\"wifi_rssi_dbm\":%ld,\"uptime_ms\":%lu,"
      "\"threshold_source\":\"bench_stage6\",\"production_ready\":false}",
      snapshot.state, snapshot.rmsG, snapshot.peakG, snapshot.sequence,
      snapshot.failedSamples, snapshot.missedPeriods, ageMs,
      sensorHealthy ? "true" : "false", WiFi.RSSI(), millis());

  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Cache-Control: no-store\r\n");
  client.print("Connection: close\r\n");
  const size_t safeBodyLength = bodyLength < 0 ? 0 :
      min(static_cast<size_t>(bodyLength), sizeof(body) - 1);
  client.printf("Content-Length: %u\r\n\r\n", safeBodyLength);
  client.write(reinterpret_cast<const uint8_t *>(body), safeBodyLength);
}

void handleClient(WiFiClient &client) {
  const uint32_t deadline = millis() + CLIENT_WAIT_MS;
  while (!client.available() && client.connected() &&
         static_cast<int32_t>(millis() - deadline) < 0) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (!client.available()) return;
  const String requestLine = client.readStringUntil('\n');
  if (requestLine.startsWith("GET /status ") || requestLine.startsWith("GET / ")) {
    sendStatus(client);
  } else {
    client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
  }
}

void telemetryTask(void *) {
  bool serverStarted = false;
  bool reconnectSelfTestTriggered = false;
  uint32_t lastConnectAttemptMs = 0;
  WiFi.mode(WIFI_STA);

  for (;;) {
    const uint32_t nowMs = millis();
    if (WiFi.status() != WL_CONNECTED) {
      serverStarted = false;
      if (nowMs - lastConnectAttemptMs >= WIFI_RETRY_MS || lastConnectAttemptMs == 0) {
        lastConnectAttemptMs = nowMs;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
    } else {
#if WIFI_RECONNECT_SELF_TEST_MS > 0
      if (!reconnectSelfTestTriggered && nowMs >= WIFI_RECONNECT_SELF_TEST_MS) {
        reconnectSelfTestTriggered = true;
        Serial.println("Telemetry self-test: forcing WiFi disconnect");
        WiFi.disconnect();
        continue;
      }
#endif
      if (!serverStarted) {
        statusServer.begin();
        serverStarted = true;
        Serial.printf("Telemetry: http://%s/status\n", WiFi.localIP().toString().c_str());
      }

      WiFiClient client = statusServer.accept();
      if (client) {
        client.setTimeout(CLIENT_WAIT_MS);
        handleClient(client);
        client.stop();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void telemetrySetup() {
  portENTER_CRITICAL(&telemetryMux);
  telemetrySnapshot = {"UNKNOWN", 0.0F, 0.0F, 0, 0, 0, 0};
  portEXIT_CRITICAL(&telemetryMux);

  BaseType_t result = xTaskCreatePinnedToCore(
      telemetryTask, "telemetry", 6144, nullptr, 1, nullptr, 0);
  if (result != pdPASS) {
    stopWithError("Telemetry: task creation failed");
  }
}
