/**
 * @file SensorsWeb.ino
 * @brief ESP32 sensor hub with I2C, web interface, and FreeRTOS tasks.
 *
 * Reads data from DHT11 (temperature & humidity), photoresistor,
 * and sound sensor (clap detection).
 * Sensor data is exposed via:
 *  - I2C slave interface
 *  - HTTP JSON API
 *  - Local web dashboard
 */

#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"
#include <Wire.h>

// -----------------------------------------------------------------------------
// I2C Configuration
// -----------------------------------------------------------------------------

#define I2C_ADDR 0x09    ///< Unique I2C slave address for ESP32
#define I2C_BUF  32      ///< Fixed I2C response buffer size

// -----------------------------------------------------------------------------
// Pin Configuration
// -----------------------------------------------------------------------------

#define DHT11_PIN 15     ///< DHT11 data pin
#define MIC_PIN   5      ///< Digital microphone / sound sensor pin
#define PHOTO_PIN 34     ///< Photoresistor analog pin (ADC)

// -----------------------------------------------------------------------------
// Wi-Fi Configuration (PRIVATE)
// -----------------------------------------------------------------------------

#define WIFI_SSID     "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// -----------------------------------------------------------------------------
// Global Objects
// -----------------------------------------------------------------------------

DHT dht(DHT11_PIN, DHT11);
WebServer server(80);

// -----------------------------------------------------------------------------
// Shared Sensor Data Structure
// -----------------------------------------------------------------------------

/**
 * @brief Shared structure for all sensor readings.
 * Protected by a mutex for thread-safe access.
 */
struct SensorData {
  float temp;                 ///< Temperature in °C
  float hum;                  ///< Humidity in %
  int   photo;                ///< Light level (ADC value)
  unsigned long lastClapMs;   ///< Timestamp of last clap event (ms)
};

SensorData data;
SemaphoreHandle_t dataMutex;

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

char i2cResp[I2C_BUF];        ///< I2C response buffer

// -----------------------------------------------------------------------------
// Sensor Task (DHT11 + Photoresistor)
// -----------------------------------------------------------------------------

/**
 * @brief Periodically reads temperature, humidity, and light level.
 */
void SensorTask(void *pv) {
  while (1) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int p   = analogRead(PHOTO_PIN);

    // Update shared data only if DHT values are valid
    if (!isnan(h) && !isnan(t)) {
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      data.temp  = t;
      data.hum   = h;
      data.photo = p;
      xSemaphoreGive(dataMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); // Run every 2 seconds
  }
}

// -----------------------------------------------------------------------------
// Sound / Clap Detection Task
// -----------------------------------------------------------------------------

unsigned long last_event = 0;

/**
 * @brief Detects clap events using a digital sound sensor.
 */
void SoundTask(void *pv) {
  pinMode(MIC_PIN, INPUT);

  while (1) {
    if (digitalRead(MIC_PIN) == LOW) {
      // Simple debounce / clap filtering
      if (millis() - last_event > 250) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        data.lastClapMs = millis();
        xSemaphoreGive(dataMutex);

        Serial.println("Clap detected");
        last_event = millis();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -----------------------------------------------------------------------------
// HTTP Handlers
// -----------------------------------------------------------------------------

/**
 * @brief Returns current sensor data in JSON format.
 */
void handleStatus() {
  xSemaphoreTake(dataMutex, portMAX_DELAY);

  unsigned long now = millis();
  float clapAgo = -1;

  if (data.lastClapMs > 0) {
    clapAgo = (now - data.lastClapMs) / 1000.0;
  }

  String json = "{";
  json += "\"temp\":"  + String(data.temp)  + ",";
  json += "\"hum\":"   + String(data.hum)   + ",";
  json += "\"photo\":" + String(data.photo) + ",";
  json += "\"clapAgo\":"+ String(clapAgo, 2);
  json += "}";

  xSemaphoreGive(dataMutex);

  server.send(200, "application/json", json);
}

// -----------------------------------------------------------------------------
// Web Interface (HTML)
// -----------------------------------------------------------------------------

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>ESP32 Sensors</title>
<style>
  body {
    font-family: Arial, sans-serif;
    background: #0f172a;
    color: #e5e7eb;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
  }

  .card {
    background: #020617;
    padding: 25px 35px;
    border-radius: 12px;
    box-shadow: 0 0 20px rgba(0,0,0,0.6);
    width: 320px;
  }

  h1 {
    text-align: center;
    margin-bottom: 20px;
  }

  .row {
    display: flex;
    justify-content: space-between;
    margin: 12px 0;
    font-size: 18px;
  }

  .clap {
    text-align: center;
    margin-top: 15px;
    font-size: 20px;
    color: #22c55e;
  }
</style>
</head>

<body>
<div class="card">
  <h1>ESP32 Sensors</h1>

  <div class="row"><span>🌡 Temperature</span><span id="temp">--</span></div>
  <div class="row"><span>💧 Humidity</span><span id="hum">--</span></div>
  <div class="row"><span>💡 Light</span><span id="photo">--</span></div>

  <div class="clap" id="clap"></div>
</div>

<script>
function updateData() {
  fetch('/status')
    .then(r => r.json())
    .then(d => {
      document.getElementById('temp').innerText =
        d.temp.toFixed(1) + ' °C';
      document.getElementById('hum').innerText =
        d.hum.toFixed(1) + ' %';
      document.getElementById('photo').innerText =
        d.photo;

      const clapEl = document.getElementById('clap');

      if (d.clapAgo < 0) {
        clapEl.innerText = '—';
      } else if (d.clapAgo < 1.5) {
        clapEl.innerText = '👏 Clap detected!';
        clapEl.style.color = '#22c55e';
      } else {
        clapEl.innerText =
          '⏱ Clap was ' + d.clapAgo.toFixed(1) + ' s ago';
        clapEl.style.color = '#94a3b8';
      }
    });
}

setInterval(updateData, 2000);
updateData();
</script>
</body>
</html>
)rawliteral";

// -----------------------------------------------------------------------------
// Web Server Task
// -----------------------------------------------------------------------------

/**
 * @brief Handles HTTP server requests in a dedicated FreeRTOS task.
 */
void WebTask(void *pv) {
  server.on("/", []() {
    server.send_P(200, "text/html", MAIN_page);
  });

  server.on("/status", handleStatus);
  server.begin();

  while (1) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -----------------------------------------------------------------------------
// I2C Callback
// -----------------------------------------------------------------------------

/**
 * @brief Sends sensor data to I2C master in a fixed-size packet.
 */
void onI2CRequest() {
  unsigned long now = millis();
  int clapSec = -1;

  xSemaphoreTake(dataMutex, portMAX_DELAY);

  if (data.lastClapMs > 0) {
    clapSec = (now - data.lastClapMs) / 1000;
    if (clapSec >= 1000) clapSec = -1;
  }

  // Clear response buffer
  memset(i2cResp, 0, I2C_BUF);

  snprintf(i2cResp, I2C_BUF,
           "T=%.1f H=%.1f P=%d C=%d",
           data.temp, data.hum, data.photo, clapSec);

  xSemaphoreGive(dataMutex);

  // Always transmit full 32-byte packet
  Wire.write((const uint8_t*)i2cResp, I2C_BUF);
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

/**
 * @brief Initializes Wi-Fi, I2C, ADC, mutex, and FreeRTOS tasks.
 */
void setup() {
  Serial.begin(115200);

  dht.begin();

  analogReadResolution(12);       // ADC range: 0–4095
  analogSetAttenuation(ADC_11db); // Up to 3.3V

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  Wire.begin(I2C_ADDR);
  Wire.onRequest(onI2CRequest);

  dataMutex = xSemaphoreCreateMutex();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(SensorTask, "Sensor", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(SoundTask,  "Sound",  2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(WebTask,    "Web",    4096, NULL, 1, NULL, 0);
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------

/**
 * @brief Main loop is unused (FreeRTOS-based design).
 */
void loop() {
  vTaskDelay(portMAX_DELAY);
}
