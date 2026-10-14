#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFi.h>
#include <cmath>

#include "web_ui.h"
#include "wifi_credentials.h"

float aht20_temp = -1.0f;
float bmp280_temp = -1.0f;
float aht20_rh = -1.0f;
float bmp280_pressure = -1.0f;
uint8_t heater_fan_pwm = 0;
uint8_t heater_pwm = 0;
uint8_t exhaust_fan_pwm = 0;

namespace {
constexpr char kHostname[] = "drybox";
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kSampleIntervalMs = 5000;
constexpr size_t kHistoryCapacity = 4096;
constexpr size_t kStreamChunkSize = 1024;
constexpr uint8_t kI2cSdaPin = 5;
constexpr uint8_t kI2cSclPin = 6;
constexpr uint8_t kHeaterPwmPin = 4;
constexpr uint8_t kExhaustFanPwmPin = 3;
constexpr uint8_t kHeaterPwmChannel = 0;
constexpr uint8_t kExhaustFanPwmChannel = 2;
constexpr uint8_t kPwmResolutionBits = 8;
constexpr uint32_t kHeaterPwmFrequencyHz = 1000;
constexpr uint32_t kExhaustFanPwmFrequencyHz = 1000;
constexpr uint8_t kOledI2cAddress = 0x3C;
constexpr uint32_t kOledRefreshMs = 2000;
constexpr uint32_t kSensorRefreshMs = 1000;
constexpr float kMinimumSafeTemperatureC = 0.0f;
constexpr float kMaximumSafeTemperatureC = 65.0f;
constexpr float kDefaultDryingSetTemperatureC = 45.0f;
constexpr uint32_t kDefaultDryingDurationMs = 30UL * 60UL * 1000UL;
constexpr uint32_t kMaximumDryingDurationMs = 24UL * 60UL * 60UL * 1000UL;
constexpr float kHeaterHysteresisC = 0.5f;
constexpr uint32_t kWifiReconnectIntervalMs = 10000;

enum class DryingState : uint8_t {
  Idle,
  Drying,
  SafetyShutdown,
  TimerExpired,
};

struct SensorSample {
  uint32_t capturedAtMs;
  float aht20Temp;
  float bmp280Temp;
  float aht20Rh;
  float bmp280Pressure;
  uint8_t heaterFanPwm;
  uint8_t heaterPwm;
  uint8_t exhaustFanPwm;
};

static_assert(sizeof(SensorSample) == 24, "Unexpected sample padding");

SensorSample history[kHistoryCapacity] __attribute__((used)){};
size_t historyHead = 0;
size_t historyCount = 0;
uint32_t nextSampleAtMs = 0;
WebServer server(kHttpPort);
U8G2_SSD1306_72X40_ER_F_HW_I2C oled(U8G2_R2, U8X8_PIN_NONE);
bool oledAvailable = false;
uint32_t nextOledUpdateMs = 0;
bool showTemperatureOnOled = true;
DryingState dryingState = DryingState::Idle;
float dryingSetTemperatureC = kDefaultDryingSetTemperatureC;
uint32_t dryingDurationMs = kDefaultDryingDurationMs;
uint32_t dryingStartedAtMs = 0;
uint32_t nextWifiReconnectAttemptMs = 0;
bool wifiWasConnected = false;
bool wifiConfigured = false;
bool webServerStarted = false;
bool mdnsStarted = false;
bool pwmOutputsReady = false;
Adafruit_AHTX0 aht20;
Adafruit_BMP280 bmp280(&Wire);
bool aht20Available = false;
bool bmp280Available = false;
uint32_t nextSensorReadMs = 0;

float averageAirTemperature();

void initializePwmOutputs() {
  pinMode(kHeaterPwmPin, OUTPUT);
  pinMode(kExhaustFanPwmPin, OUTPUT);
  digitalWrite(kHeaterPwmPin, LOW);
  digitalWrite(kExhaustFanPwmPin, LOW);

  if (ledcSetup(kHeaterPwmChannel, kHeaterPwmFrequencyHz, kPwmResolutionBits) == 0
      || ledcSetup(kExhaustFanPwmChannel, kExhaustFanPwmFrequencyHz, kPwmResolutionBits) == 0) {
    return;
  }

  ledcWrite(kHeaterPwmChannel, 0);
  ledcWrite(kExhaustFanPwmChannel, 0);
  ledcAttachPin(kHeaterPwmPin, kHeaterPwmChannel);
  ledcAttachPin(kExhaustFanPwmPin, kExhaustFanPwmChannel);
  pwmOutputsReady = true;
}

void initializeSensors() {
  aht20Available = aht20.begin(&Wire);
  if (aht20Available) {
    Serial.println("AHT20 initialized on I2C address 0x38.");
  } else {
    Serial.println("AHT20 not detected; using fallback sensor values.");
  }

  bmp280Available = bmp280.begin(BMP280_ADDRESS);
  if (!bmp280Available) {
    bmp280Available = bmp280.begin(BMP280_ADDRESS_ALT);
  }
  if (bmp280Available) {
    bmp280.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X4,
      Adafruit_BMP280::STANDBY_MS_125
    );
    Serial.println("BMP280 initialized on I2C address 0x76/0x77.");
  } else {
    Serial.println("BMP280 not detected; using fallback sensor values.");
  }
}

void readSensors(uint32_t now) {
  if (static_cast<int32_t>(now - nextSensorReadMs) < 0) {
    return;
  }
  nextSensorReadMs = now + kSensorRefreshMs;

  if (aht20Available) {
    sensors_event_t humidityEvent;
    sensors_event_t temperatureEvent;
    if (aht20.getEvent(&humidityEvent, &temperatureEvent)
        && std::isfinite(temperatureEvent.temperature)
        && std::isfinite(humidityEvent.relative_humidity)) {
      aht20_temp = temperatureEvent.temperature;
      aht20_rh = humidityEvent.relative_humidity;
    } else {
      aht20_temp = -1.0f;
      aht20_rh = -1.0f;
    }
  } else {
    aht20_temp = -1.0f;
    aht20_rh = -1.0f;
  }

  if (bmp280Available) {
    const float temperature = bmp280.readTemperature();
    const float pressurePa = bmp280.readPressure();
    if (std::isfinite(temperature) && std::isfinite(pressurePa) && pressurePa > 0.0f) {
      bmp280_temp = temperature;
      bmp280_pressure = pressurePa / 100.0f;
    } else {
      bmp280_temp = -1.0f;
      bmp280_pressure = -1.0f;
    }
  } else {
    bmp280_temp = -1.0f;
    bmp280_pressure = -1.0f;
  }
}

bool temperaturesAreSafe() {
  return std::isfinite(aht20_temp)
    && std::isfinite(bmp280_temp)
    && aht20_temp >= kMinimumSafeTemperatureC
    && aht20_temp <= kMaximumSafeTemperatureC
    && bmp280_temp >= kMinimumSafeTemperatureC
    && bmp280_temp <= kMaximumSafeTemperatureC;
}

void forceOutputsOff() {
  heater_fan_pwm = 0;
  heater_pwm = 0;
  exhaust_fan_pwm = 0;
  if (pwmOutputsReady) {
    ledcWrite(kHeaterPwmChannel, 0);
    ledcWrite(kExhaustFanPwmChannel, 0);
  }
}

const char* dryingStateName() {
  switch (dryingState) {
    case DryingState::Drying: return "drying";
    case DryingState::SafetyShutdown: return "safety";
    case DryingState::TimerExpired: return "timer_expired";
    case DryingState::Idle:
    default: return "idle";
  }
}

const char* dryingSafetyReason() {
  if (dryingState == DryingState::SafetyShutdown) {
    if (!pwmOutputsReady) {
      return "PWM outputs unavailable";
    }
    return "Temperature outside 0-65 C";
  }
  if (dryingState == DryingState::TimerExpired) {
    return "Drying cycle timer expired";
  }
  return "";
}

uint32_t dryingRemainingSeconds(uint32_t now) {
  if (dryingState != DryingState::Drying) {
    return 0;
  }
  const uint32_t elapsed = now - dryingStartedAtMs;
  if (elapsed >= dryingDurationMs) {
    return 0;
  }
  return (dryingDurationMs - elapsed + 999) / 1000;
}

void updateDryingControl(uint32_t now) {
  if (!pwmOutputsReady) {
    forceOutputsOff();
    if (dryingState == DryingState::Drying) {
      dryingState = DryingState::SafetyShutdown;
    }
    return;
  }

  if (!temperaturesAreSafe()) {
    forceOutputsOff();
    if (dryingState == DryingState::Drying) {
      dryingState = DryingState::SafetyShutdown;
    }
    return;
  }

  if (dryingState != DryingState::Drying) {
    forceOutputsOff();
    return;
  }

  if (now - dryingStartedAtMs >= dryingDurationMs) {
    dryingState = DryingState::Idle;
    forceOutputsOff();
    return;
  }

  const float averageTemperature = averageAirTemperature();
  if (!std::isfinite(averageTemperature)) {
    dryingState = DryingState::SafetyShutdown;
    forceOutputsOff();
    return;
  }

  heater_fan_pwm = 255;
  exhaust_fan_pwm = 255;
  if (averageTemperature <= dryingSetTemperatureC - kHeaterHysteresisC) {
    heater_pwm = 255;
  } else if (averageTemperature >= dryingSetTemperatureC + kHeaterHysteresisC) {
    heater_pwm = 0;
  }
  ledcWrite(kExhaustFanPwmChannel, exhaust_fan_pwm);
  ledcWrite(kHeaterPwmChannel, heater_pwm);
}

float averageAirTemperature() {
  float sum = 0.0f;
  uint8_t validReadings = 0;
  if (std::isfinite(aht20_temp)) {
    sum += aht20_temp;
    ++validReadings;
  }
  if (std::isfinite(bmp280_temp)) {
    sum += bmp280_temp;
    ++validReadings;
  }
  return validReadings == 0 ? NAN : sum / validReadings;
}

void updateOled() {
  if (!oledAvailable || static_cast<int32_t>(millis() - nextOledUpdateMs) < 0) {
    return;
  }
  nextOledUpdateMs = millis() + kOledRefreshMs;

  char reading[16];
  if (showTemperatureOnOled) {
    const float averageTemperature = averageAirTemperature();
    if (std::isfinite(averageTemperature) && averageTemperature >= 0.0f && averageTemperature <= 125.0f) {
      snprintf(reading, sizeof(reading), "%dC", static_cast<int>(std::lround(averageTemperature)));
    } else {
      strlcpy(reading, "--C", sizeof(reading));
    }
  } else if (std::isfinite(aht20_rh) && aht20_rh >= 0.0f && aht20_rh <= 100.0f) {
    snprintf(reading, sizeof(reading), "%d%%", static_cast<int>(std::lround(aht20_rh)));
  } else {
    strlcpy(reading, "--%", sizeof(reading));
  }
  showTemperatureOnOled = !showTemperatureOnOled;

  oled.clearBuffer();
  oled.setFont(u8g2_font_logisoso28_tr);
  if (oled.getStrWidth(reading) > oled.getDisplayWidth()) {
    oled.setFont(u8g2_font_helvB24_tr);
  }
  if (oled.getStrWidth(reading) > oled.getDisplayWidth()) {
    oled.setFont(u8g2_font_helvB18_tr);
  }
  const uint8_t x = (oled.getDisplayWidth() - oled.getStrWidth(reading)) / 2;
  const uint8_t y = (oled.getDisplayHeight() + oled.getAscent() + oled.getDescent()) / 2;
  oled.drawStr(x, y, reading);
  oled.sendBuffer();
}

void startWebServer();

void maintainWiFi(uint32_t now) {
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiConnected) {
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      Serial.printf("Wi-Fi reconnected: http://%s\n", WiFi.localIP().toString().c_str());
      startWebServer();
    }
    return;
  }

  if (wifiWasConnected) {
    wifiWasConnected = false;
    Serial.println("Wi-Fi connection lost; reconnecting...");
    if (mdnsStarted) {
      MDNS.end();
      mdnsStarted = false;
    }
    nextWifiReconnectAttemptMs = now;
  }

  if (wifiConfigured && static_cast<int32_t>(now - nextWifiReconnectAttemptMs) >= 0) {
    Serial.println("Wi-Fi reconnect attempt.");
    WiFi.reconnect();
    nextWifiReconnectAttemptMs = now + kWifiReconnectIntervalMs;
  }
}

void formatJsonFloat(char* destination, size_t capacity, float value, uint8_t decimals) {
  if (std::isfinite(value)) {
    snprintf(destination, capacity, "%.*f", decimals, static_cast<double>(value));
  } else {
    strlcpy(destination, "null", capacity);
  }
}

void formatCsvFloat(char* destination, size_t capacity, float value, uint8_t decimals) {
  if (std::isfinite(value)) {
    snprintf(destination, capacity, "%.*f", decimals, static_cast<double>(value));
  } else {
    destination[0] = '\0';
  }
}

void captureSample(uint32_t capturedAtMs) {
  history[historyHead] = {
    capturedAtMs,
    aht20_temp,
    bmp280_temp,
    aht20_rh,
    bmp280_pressure,
    heater_fan_pwm,
    heater_pwm,
    exhaust_fan_pwm,
  };
  historyHead = (historyHead + 1) % kHistoryCapacity;
  if (historyCount < kHistoryCapacity) {
    ++historyCount;
  }
}

size_t oldestHistoryIndex() {
  return historyCount == kHistoryCapacity ? historyHead : 0;
}

const SensorSample* latestSample() {
  if (historyCount == 0) {
    return nullptr;
  }
  const size_t index = historyHead == 0 ? kHistoryCapacity - 1 : historyHead - 1;
  return &history[index];
}

size_t requestedHistoryCount() {
  if (!server.hasArg("limit")) {
    return historyCount;
  }
  const long requested = server.arg("limit").toInt();
  if (requested <= 0) {
    return historyCount;
  }
  return min(historyCount, static_cast<size_t>(requested));
}

void sendChunk(String& chunk) {
  if (!chunk.isEmpty()) {
    server.sendContent(chunk);
    chunk.clear();
  }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html; charset=utf-8", drybox_ui::kIndexHtml);
}

void handleDryingControl() {
  if (server.hasArg("set_temperature")) {
    const float requestedTemperature = server.arg("set_temperature").toFloat();
    if (!std::isfinite(requestedTemperature)
        || requestedTemperature < kMinimumSafeTemperatureC
        || requestedTemperature > kMaximumSafeTemperatureC) {
      server.send(400, "application/json", "{\"error\":\"set_temperature must be between 0 and 65 C\"}");
      return;
    }
    dryingSetTemperatureC = requestedTemperature;
  }

  if (server.hasArg("duration_minutes")) {
    const long requestedMinutes = server.arg("duration_minutes").toInt();
    if (requestedMinutes <= 0 || requestedMinutes > static_cast<long>(kMaximumDryingDurationMs / (60UL * 1000UL))) {
      server.send(400, "application/json", "{\"error\":\"duration_minutes must be between 1 and 1440\"}");
      return;
    }
    dryingDurationMs = static_cast<uint32_t>(requestedMinutes) * 60UL * 1000UL;
  }

  const String action = server.arg("action");
  if (action == "start") {
    if (!temperaturesAreSafe()) {
      dryingState = DryingState::SafetyShutdown;
      forceOutputsOff();
      server.send(409, "application/json", "{\"error\":\"Both temperatures must be within 0-65 C before starting\"}");
      return;
    }
    dryingState = DryingState::Drying;
    dryingStartedAtMs = millis();
  } else if (action == "stop") {
    dryingState = DryingState::Idle;
    forceOutputsOff();
  }

  updateDryingControl(millis());
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleStatus() {
  const uint32_t now = millis();
  const SensorSample* latest = latestSample();
  const uint32_t historySpanSeconds = historyCount > 1
    ? (now - history[oldestHistoryIndex()].capturedAtMs) / 1000
    : 0;

  String json;
  json.reserve(640);
  json += F("{\"hostname\":\"drybox.local\",\"ip\":\"");
  json += WiFi.localIP().toString();
  json += F("\",\"rssi_dbm\":");
  json += WiFi.RSSI();
  json += F(",\"uptime_s\":");
  json += now / 1000;
  json += F(",\"free_heap_bytes\":");
  json += ESP.getFreeHeap();
  json += F(",\"sample_interval_s\":");
  json += kSampleIntervalMs / 1000;
  json += F(",\"history_capacity\":");
  json += kHistoryCapacity;
  json += F(",\"history_bytes\":");
  json += sizeof(history);
  json += F(",\"sample_count\":");
  json += historyCount;
  json += F(",\"history_span_s\":");
  json += historySpanSeconds;
  json += F(",\"maximum_history_s\":");
  json += (kHistoryCapacity * kSampleIntervalMs) / 1000;
  json += F(",\"drying\":{\"state\":\"");
  json += dryingStateName();
  json += F("\",\"set_temperature_c\":");
  json += dryingSetTemperatureC;
  json += F(",\"duration_s\":");
  json += dryingDurationMs / 1000;
  json += F(",\"remaining_s\":");
  json += dryingRemainingSeconds(now);
  json += F(",\"temperatures_safe\":");
  json += temperaturesAreSafe() ? "true" : "false";
  json += F(",\"safety_reason\":\"");
  json += dryingSafetyReason();
  json += F("\"}");
  json += F(",\"latest\":");

  if (latest == nullptr) {
    json += F("null");
  } else {
    char ahtTemp[20];
    char bmpTemp[20];
    char humidity[20];
    char pressure[20];
    formatJsonFloat(ahtTemp, sizeof(ahtTemp), latest->aht20Temp, 2);
    formatJsonFloat(bmpTemp, sizeof(bmpTemp), latest->bmp280Temp, 2);
    formatJsonFloat(humidity, sizeof(humidity), latest->aht20Rh, 2);
    formatJsonFloat(pressure, sizeof(pressure), latest->bmp280Pressure, 1);
    char latestJson[320];
    snprintf(
      latestJson,
      sizeof(latestJson),
      "{\"age_s\":%lu,\"aht20_temp\":%s,\"bmp280_temp\":%s,\"aht20_rh\":%s,\"bmp280_pressure\":%s,\"heater_fan_pwm\":%u,\"heater_pwm\":%u,\"exhaust_fan_pwm\":%u}",
      static_cast<unsigned long>((now - latest->capturedAtMs) / 1000),
      ahtTemp,
      bmpTemp,
      humidity,
      pressure,
      latest->heaterFanPwm,
      latest->heaterPwm,
      latest->exhaustFanPwm
    );
    json += latestJson;
  }

  json += '}';
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleHistory() {
  const uint32_t now = millis();
  const size_t count = requestedHistoryCount();
  const size_t skipped = historyCount - count;
  size_t index = (oldestHistoryIndex() + skipped) % kHistoryCapacity;

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"sample_interval_s\":");
  server.sendContent(String(kSampleIntervalMs / 1000));
  server.sendContent(",\"samples\":[");

  String chunk;
  chunk.reserve(kStreamChunkSize);
  for (size_t offset = 0; offset < count; ++offset) {
    const SensorSample& sample = history[index];
    char ahtTemp[20];
    char bmpTemp[20];
    char humidity[20];
    char pressure[20];
    formatJsonFloat(ahtTemp, sizeof(ahtTemp), sample.aht20Temp, 2);
    formatJsonFloat(bmpTemp, sizeof(bmpTemp), sample.bmp280Temp, 2);
    formatJsonFloat(humidity, sizeof(humidity), sample.aht20Rh, 2);
    formatJsonFloat(pressure, sizeof(pressure), sample.bmp280Pressure, 1);

    char row[320];
    const int rowLength = snprintf(
      row,
      sizeof(row),
      "%s{\"age_s\":%lu,\"aht20_temp\":%s,\"bmp280_temp\":%s,\"aht20_rh\":%s,\"bmp280_pressure\":%s,\"heater_fan_pwm\":%u,\"heater_pwm\":%u,\"exhaust_fan_pwm\":%u}",
      offset == 0 ? "" : ",",
      static_cast<unsigned long>((now - sample.capturedAtMs) / 1000),
      ahtTemp,
      bmpTemp,
      humidity,
      pressure,
      sample.heaterFanPwm,
      sample.heaterPwm,
      sample.exhaustFanPwm
    );

    if (chunk.length() + rowLength >= kStreamChunkSize) {
      sendChunk(chunk);
    }
    chunk.concat(row, static_cast<unsigned int>(rowLength));
    index = (index + 1) % kHistoryCapacity;
  }
  sendChunk(chunk);
  server.sendContent("]}");
  server.sendContent("");
}

void handleHistoryCsv() {
  const uint32_t now = millis();
  size_t index = oldestHistoryIndex();

  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Disposition", "attachment; filename=\"drybox-history.csv\"");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv; charset=utf-8", "");
  server.sendContent("age_seconds,aht20_temp_c,bmp280_temp_c,aht20_rh_percent,bmp280_pressure_hpa,heater_fan_pwm,heater_pwm,exhaust_fan_pwm\r\n");

  String chunk;
  chunk.reserve(kStreamChunkSize);
  for (size_t offset = 0; offset < historyCount; ++offset) {
    const SensorSample& sample = history[index];
    char ahtTemp[20];
    char bmpTemp[20];
    char humidity[20];
    char pressure[20];
    formatCsvFloat(ahtTemp, sizeof(ahtTemp), sample.aht20Temp, 2);
    formatCsvFloat(bmpTemp, sizeof(bmpTemp), sample.bmp280Temp, 2);
    formatCsvFloat(humidity, sizeof(humidity), sample.aht20Rh, 2);
    formatCsvFloat(pressure, sizeof(pressure), sample.bmp280Pressure, 1);

    char row[192];
    const int rowLength = snprintf(
      row,
      sizeof(row),
      "%lu,%s,%s,%s,%s,%u,%u,%u\r\n",
      static_cast<unsigned long>((now - sample.capturedAtMs) / 1000),
      ahtTemp,
      bmpTemp,
      humidity,
      pressure,
      sample.heaterFanPwm,
      sample.heaterPwm,
      sample.exhaustFanPwm
    );

    if (chunk.length() + rowLength >= kStreamChunkSize) {
      sendChunk(chunk);
    }
    chunk.concat(row, static_cast<unsigned int>(rowLength));
    index = (index + 1) % kHistoryCapacity;
  }
  sendChunk(chunk);
  server.sendContent("");
}

void connectToWiFi() {
  wifiConfigured = strcmp(drybox_credentials::kSsid, "YOUR_WIFI_SSID") != 0;
  if (!wifiConfigured) {
    Serial.println("Set Wi-Fi credentials in include/wifi_credentials.h.");
    return;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(kHostname);
  WiFi.begin(drybox_credentials::kSsid, drybox_credentials::kPassword);
  nextWifiReconnectAttemptMs = millis() + kWifiReconnectIntervalMs;
  Serial.println("Connecting to Wi-Fi in the background...");
}

void startWebServer() {
  if (MDNS.begin(kHostname)) {
    mdnsStarted = true;
    MDNS.setInstanceName("Drybox Controller");
    MDNS.addService("http", "tcp", kHttpPort);
    Serial.println("mDNS: http://drybox.local");
  } else {
    Serial.println("mDNS failed; use the printed IP address.");
  }

  if (webServerStarted) {
    return;
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/drying", HTTP_GET, handleDryingControl);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/api/history.csv", HTTP_GET, handleHistoryCsv);
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
  });
  server.begin();
  webServerStarted = true;
}
}  // namespace

void setup() {
  initializePwmOutputs();
  Serial.begin(115200);
  delay(5000);
  Serial.println("Drybox booting...");

  Wire.begin(kI2cSdaPin, kI2cSclPin, 400000);
  oled.setI2CAddress(kOledI2cAddress << 1);
  oled.setBusClock(400000);
  oled.begin();
  oled.setContrast(150);
  oledAvailable = true;
  Serial.println("Onboard OLED initialized on SDA GPIO 5 / SCL GPIO 6.");

  initializeSensors();
  const uint32_t now = millis();
  readSensors(now);
  captureSample(now);
  nextSampleAtMs = now + kSampleIntervalMs;
  updateOled();
  connectToWiFi();
}

void loop() {
  maintainWiFi(millis());
  if (webServerStarted && wifiWasConnected) {
    server.handleClient();
  }
  updateOled();

  const uint32_t now = millis();
  readSensors(now);
  updateDryingControl(now);
  if (static_cast<int32_t>(now - nextSampleAtMs) >= 0) {
    captureSample(now);
    nextSampleAtMs = now + kSampleIntervalMs;
  }

  delay(2);
}
