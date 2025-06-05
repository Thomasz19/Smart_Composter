/******************************************************************************
 * @file    network_manager.cpp
 * @author  Thomas Zoldowski
 * @date    June 4, 2025
 * @brief   Wi-Fi connection and Ticker-driven sensor-data HTTPS POST
 *          (manual HTTP, no ArduinoHttpClient).
 ******************************************************************************/

#include <Arduino.h>
//#include <WiFi.h>             
// #include <Ticker.h>
// #include "logic/sensor_manager.h"
// #include "logic/network_manager.h"
// #include "config.h"

// // Override via platformio.ini or defaults here:
// #ifndef WIFI_SSID
//   #define WIFI_SSID "The Gentlemen "
// #endif
// #ifndef WIFI_PW
//   #define WIFI_PW   "Thomas61476"
// #endif
// #ifndef API_HOST
//   #define API_HOST "txhj3ekbrg.execute-api.us-east-2.amazonaws.com"
// #endif
// #ifndef API_PATH
//   #define API_PATH "/prod/readings"
// #endif
// static const uint16_t API_PORT = 443;  // HTTPS port

// // Prototype function
// static void sendSensorData();

// // Interval between uploads in milliseconds (60 000 ms = 60 s)
// static const unsigned long UPLOAD_INTERVAL_MS = 60000UL;

// // Create a single Ticker instance (uninitialized for now).
// //static Ticker sensorTicker;
// static Ticker sensorTicker(sendSensorData, UPLOAD_INTERVAL_MS, 0, MILLIS);

// /// Fill buf (length ≥32) with current UTC in ISO-8601 (e.g. "2025-06-04T15:04:00Z")
// static void getIsoTimestamp(char *buf, size_t len) {
//   time_t now = time(nullptr);
//   struct tm *gm = gmtime(&now);
//   strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", gm);
// }

// /// Called by the Ticker once per interval to read sensors + HTTPS-POST JSON.
// static void sendSensorData() {

//   float temps[3], hums[3], o2val = NAN;

// #ifdef SIMULATION_MODE
//   // ---------------- Simulated data path ----------------
//   for (int i = 0; i < 3; i++) {
//     // Pick a random temperature between 20.0°F and 30.0°F for example
//     float tempF = 20.0f + (random(0, 101) * 0.1f);   // 20.0 -> 30.0
//     float humPct = 40.0f + (random(0, 201) * 0.1f);  // 40.0% -> 60.0%

//     temps[i] = tempF;
//     hums[i]  = humPct;
//   }
//   // Simulate O₂ around 20.9%
//   o2val = 20.0f + (random(0, 21) * 0.01f);  // 20.00 -> 20.20
// #else
//   // ---------------- Real‐sensor path ----------------
//   ConnectionStatus st = sensor_manager_get_connection_status();
//   for (int i = 0; i < 3; i++) {
//     if (st.sensor[i]) {
//       float tC = sensor_manager_get_temperature(i);
//       temps[i] = (tC * 9.0f / 5.0f) + 32.0f;
//       hums[i]  = sensor_manager_get_humidity(i);
//     } else {
//       temps[i] = NAN;
//       hums[i]  = NAN;
//     }
//   }
//   // If you have an O₂ sensor, do:
//   //     o2val = sensor_manager_get_o2();
//   // otherwise leave as NAN (it will become 0.0 in the JSON below)
// #endif

//   char tsbuf[32];
//   getIsoTimestamp(tsbuf, sizeof(tsbuf));

//   // Build JSON payload
//   char json[512];
//   int len = snprintf(json, sizeof(json),
//     "{"
//       "\"deviceId\":\"GIGA-001\","
//       "\"timestamp\":\"%s\","
//       "\"sensor\":["
//         "{\"id\":0,\"temp\":%.1f,\"hum\":%.1f},"
//         "{\"id\":1,\"temp\":%.1f,\"hum\":%.1f},"
//         "{\"id\":2,\"temp\":%.1f,\"hum\":%.1f}"
//       "],"
//       "\"o2\":%.1f"
//     "}",
//     tsbuf,
//     isnan(temps[0]) ? 0.0f : temps[0],
//     isnan(hums[0])  ? 0.0f : hums[0],
//     isnan(temps[1]) ? 0.0f : temps[1],
//     isnan(hums[1])  ? 0.0f : hums[1],
//     isnan(temps[2]) ? 0.0f : temps[2],
//     isnan(hums[2])  ? 0.0f : hums[2],
//     isnan(o2val)    ? 0.0f : o2val
//   );
//   if (len < 0 || len >= (int)sizeof(json)) {
//     Serial.println("[Error] JSON buffer overflow");
//     return;
//   }

//   // 1) Create a TLS client (WiFiClientSecure)
//   WiFiClient tlsClient;
//   // For testing, skip certificate verification:
//   //tlsClient.setInsecure();
//   // (In production, call tlsClient.setCACert(rootCACertPEM) with your CA)

//   // 2) Connect to the host on port 443
//   if (!tlsClient.connect(API_HOST, API_PORT)) {
//     Serial.println("[HTTPS] Connection failed");
//     return;
//   }

//   // 3) Manually format and send the HTTP/1.1 POST request
//   String request =
//     String("POST ") + API_PATH + " HTTP/1.1\r\n" +
//     "Host: " + API_HOST + "\r\n" +
//     "Content-Type: application/json\r\n" +
//     "Content-Length: " + String(len) + "\r\n" +
//     "Connection: close\r\n\r\n" +
//     String(json);

//   tlsClient.print(request);

//   // 4) Read the status line (e.g. "HTTP/1.1 200 OK")
//   String statusLine = tlsClient.readStringUntil('\n');
//   statusLine.trim();
//   Serial.print("[HTTPS] ");
//   Serial.println(statusLine);

//   // 5) Skip HTTP headers until a blank line
//   while (tlsClient.connected()) {
//     String line = tlsClient.readStringUntil('\n');
//     if (line == "\r" || line == "\n" || line.length() == 0) break;
//   }

//   // 6) Optionally read any response body
//   String body = tlsClient.readString();
//   if (body.length()) {
//     Serial.print("[HTTPS] Body: ");
//     Serial.println(body);
//   }

//   tlsClient.stop();
// }

// void network_init_and_start() {
//   // 1) Connect to Wi-Fi using the built-in WiFi.h
//   Serial.print("[WiFi] Connecting to ");
//   Serial.print(WIFI_SSID);
//   Serial.print(" …");
//   WiFi.begin(WIFI_SSID, WIFI_PW);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.print("\n[WiFi] Connected (IP=");
//   Serial.print(WiFi.localIP());
//   Serial.println(")");

//   // 2) Ensure sensor_manager_init() has already been called

//   // 3) Send one payload immediately
//   sendSensorData();

//   // 4) Construct the Ticker:
//   //    callback=sendSensorData, interval=UPLOAD_INTERVAL_MS, repeat=0 (infinite), resolution=MILLIS
  
//   sensorTicker.start();
// }

// void network_update() {
//   // Must call update() each loop so Ticker can check if it's time to call sendSensorData()
//   sensorTicker.update();
// }
