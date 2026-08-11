#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#elif __has_include("../stage_02_wifi_test/secrets.h")
#include "../stage_02_wifi_test/secrets.h"
#else
#error "Create secrets.h from secrets.example.h"
#endif

void integratedSetup();
void integratedEarlySetup();
void integratedStateUpdated(const char *stateValue, float rmsValue, float peakValue,
                            uint32_t failedValue, uint32_t missedValue);

#define MONITOR_EARLY_SETUP() integratedEarlySetup()
#define MONITOR_EXTRA_SETUP() integratedSetup()
#define MONITOR_STATE_UPDATED(stateValue, rmsValue, peakValue, failedValue, missedValue) \
  integratedStateUpdated(stateValue, rmsValue, peakValue, failedValue, missedValue)

#include "../stage_07_state_classifier/stage_07_state_classifier.ino"

namespace {
constexpr uint8_t LIGHT_1_PIN = 34;
constexpr uint8_t LIGHT_2_PIN = 35;
constexpr uint8_t BUZZER_PIN = 25;
constexpr uint8_t LIGHT_SAMPLES = 16;
constexpr uint16_t HTTP_PORT = 80;
constexpr uint32_t WIFI_RETRY_MS = 10000;
constexpr uint32_t CLIENT_WAIT_MS = 50;

struct WifiCredential {
  const char *ssid;
  const char *password;
};

constexpr WifiCredential WIFI_NETWORKS[] = {
  {WIFI_FACTORY_SSID, WIFI_FACTORY_PASSWORD},
  {WIFI_SSID, WIFI_PASSWORD},
};
constexpr size_t WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

struct IntegratedSnapshot {
  char state[8];
  float rmsG;
  float peakG;
  uint16_t light1Raw;
  uint16_t light2Raw;
  uint32_t failedSamples;
  uint32_t missedPeriods;
  uint32_t sequence;
  uint32_t updatedMs;
};

IntegratedSnapshot snapshot = {
  "UNKNOWN", 0.0F, 0.0F, 0, 0, 0, 0, 0, 0
};
portMUX_TYPE snapshotMux = portMUX_INITIALIZER_UNLOCKED;
WiFiServer statusServer(HTTP_PORT);
volatile uint32_t networkTaskStartedMs = 0;
volatile uint32_t networkLoopCount = 0;
volatile uint32_t networkAttemptCount = 0;
volatile int networkStatusCode = -1;

uint16_t readAveragedLight(uint8_t pin) {
  analogRead(pin);
  uint32_t sum = 0;
  for (uint8_t i = 0; i < LIGHT_SAMPLES; ++i) {
    sum += analogRead(pin);
  }
  return static_cast<uint16_t>(sum / LIGHT_SAMPLES);
}

void copySnapshot(IntegratedSnapshot &destination) {
  portENTER_CRITICAL(&snapshotMux);
  destination = snapshot;
  portEXIT_CRITICAL(&snapshotMux);
}

void sendStatus(WiFiClient &client) {
  IntegratedSnapshot current;
  copySnapshot(current);
  const uint32_t ageMs = millis() - current.updatedMs;
  const bool sensorHealthy = current.failedSamples == 0 && current.missedPeriods == 0 &&
                             ageMs < 3000;

  char body[640];
  const int bodyLength = snprintf(
      body, sizeof(body),
      "{\"device\":\"equipment-monitor-01\",\"state\":\"%s\"," 
      "\"vibration_rms_g\":%.5f,\"peak_g\":%.5f,"
      "\"light_1_raw\":%u,\"light_2_raw\":%u,\"sequence\":%lu,"
      "\"sample_failed\":%lu,\"sample_missed\":%lu,\"sample_age_ms\":%lu,"
      "\"sensor_healthy\":%s,\"wifi_rssi_dbm\":%ld,\"uptime_ms\":%lu,"
      "\"threshold_source\":\"bench_stage6\",\"light_calibrated\":false,"
      "\"production_ready\":false}",
      current.state, current.rmsG, current.peakG,
      current.light1Raw, current.light2Raw, current.sequence,
      current.failedSamples, current.missedPeriods, ageMs,
      sensorHealthy ? "true" : "false", WiFi.RSSI(), millis());

  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Cache-Control: no-store\r\n");
  client.print("Connection: close\r\n");
  const size_t safeLength = bodyLength < 0 ? 0 :
      min(static_cast<size_t>(bodyLength), sizeof(body) - 1);
  client.printf("Content-Length: %u\r\n\r\n", safeLength);
  client.write(reinterpret_cast<const uint8_t *>(body), safeLength);
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

void networkTask(void *) {
  bool serverStarted = false;
  uint32_t lastConnectAttemptMs = 0;
  size_t nextNetworkIndex = 0;
  networkTaskStartedMs = millis();
  WiFi.mode(WIFI_STA);

  for (;;) {
    ++networkLoopCount;
    networkStatusCode = static_cast<int>(WiFi.status());
    const uint32_t nowMs = millis();
    if (WiFi.status() != WL_CONNECTED) {
      serverStarted = false;
      if (nowMs - lastConnectAttemptMs >= WIFI_RETRY_MS || lastConnectAttemptMs == 0) {
        lastConnectAttemptMs = nowMs;
        WiFi.disconnect();
        const WifiCredential &network = WIFI_NETWORKS[nextNetworkIndex];
        ++networkAttemptCount;
        Serial.printf("WiFi: trying SSID %s\n", network.ssid);
        WiFi.begin(network.ssid, network.password);
        nextNetworkIndex = (nextNetworkIndex + 1) % WIFI_NETWORK_COUNT;
      }
    } else {
      if (!serverStarted) {
        statusServer.begin();
        serverStarted = true;
        Serial.printf("Integrated telemetry: http://%s/status\n",
                      WiFi.localIP().toString().c_str());
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
}

void integratedStateUpdated(const char *stateValue, float rmsValue, float peakValue,
                            uint32_t failedValue, uint32_t missedValue) {
  const uint16_t light1 = readAveragedLight(LIGHT_1_PIN);
  const uint16_t light2 = readAveragedLight(LIGHT_2_PIN);

  portENTER_CRITICAL(&snapshotMux);
  snprintf(snapshot.state, sizeof(snapshot.state), "%s", stateValue);
  snapshot.rmsG = rmsValue;
  snapshot.peakG = peakValue;
  snapshot.light1Raw = light1;
  snapshot.light2Raw = light2;
  snapshot.failedSamples = failedValue;
  snapshot.missedPeriods = missedValue;
  ++snapshot.sequence;
  snapshot.updatedMs = millis();
  portEXIT_CRITICAL(&snapshotMux);

  Serial.printf("lights:s1=%u,s2=%u,wifi_task_ms=%lu,wifi_loops=%lu,wifi_attempts=%lu,wifi_status=%d\n",
                light1, light2,
                static_cast<unsigned long>(networkTaskStartedMs),
                static_cast<unsigned long>(networkLoopCount),
                static_cast<unsigned long>(networkAttemptCount),
                networkStatusCode);
}

void integratedEarlySetup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void integratedSetup() {
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_1_PIN, ADC_11db);
  analogSetPinAttenuation(LIGHT_2_PIN, ADC_11db);

  portENTER_CRITICAL(&snapshotMux);
  snapshot = {"UNKNOWN", 0.0F, 0.0F, 0, 0, 0, 0, 0, 0};
  portEXIT_CRITICAL(&snapshotMux);

  const BaseType_t result = xTaskCreatePinnedToCore(
      networkTask, "integrated-network", 6144, nullptr, 1, nullptr, 0);
  if (result != pdPASS) {
    stopWithError("Integrated telemetry: task creation failed");
  }
}
