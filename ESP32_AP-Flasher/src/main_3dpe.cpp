/**
 * main_3dpe.cpp
 *
 * Simplified main file for 3DPE XIAO ESP32C3 ePaper display device
 * This is a standalone firmware that connects to WiFi and displays device info
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "display_3dpe.h"

// WiFi connection timeout (30 seconds)
#define WIFI_TIMEOUT_MS 30000

// Heartbeat interval (60 seconds)
#define HEARTBEAT_INTERVAL_MS 60000

/**
 * Send heartbeat to ESL Manager server
 * POST /api/devices/heartbeat
 */
bool sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String(SERVER_URL) + "/api/devices/heartbeat";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  String payload = "{";
  payload += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
  payload += "\"signal_strength\":" + String(WiFi.RSSI()) + ",";
  payload += "\"firmware_version\":\"1.0.0\",";
  payload += "\"metadata\":{";
  payload += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  payload += "\"device_type\":\"" + String(DEVICE_TYPE) + "\"";
  payload += "}}";

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.println("Heartbeat sent successfully");
    http.end();
    return true;
  } else {
    Serial.printf("Heartbeat failed: %d\n", httpCode);
    http.end();
    return false;
  }
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);  // Wait for serial to initialize

  Serial.println("\n\n=================================");
  Serial.println("3DPE Smart Manufacturing ESL");
  Serial.println("XIAO ESP32C3 + 2.9\" ePaper");
  Serial.println("=================================");

  // Print device info
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Device Type: ");
  Serial.println(DEVICE_TYPE);
  Serial.print("Server URL: ");
  Serial.println(SERVER_URL);

  // Connect to WiFi
  Serial.printf("\nConnecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("WiFi connection failed - continuing in offline mode");
  }

  // Display 3DPE startup screen
  Serial.println("\nDisplaying startup screen...");
  Display3DPE::showStartupScreen();
  Serial.println("Startup screen complete");

  Serial.println("\n=================================");
  Serial.println("Device ready - entering sleep mode");
  Serial.println("=================================");
}

void loop() {
  delay(HEARTBEAT_INTERVAL_MS);

  if (WiFi.status() == WL_CONNECTED) {
    sendHeartbeat();
  } else {
    // Try to reconnect
    Serial.println("WiFi disconnected, attempting reconnect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
  }
}
