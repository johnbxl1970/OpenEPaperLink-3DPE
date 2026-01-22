/**
 * main_3dpe.cpp
 *
 * Simplified main file for 3DPE XIAO ESP32C3 ePaper display device
 * This is a standalone firmware that connects to WiFi and displays device info
 */

#include <Arduino.h>
#include <WiFi.h>
#include "display_3dpe.h"

// WiFi connection timeout (30 seconds)
#define WIFI_TIMEOUT_MS 30000

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
  // Simple heartbeat - device is mostly idle
  // In production, this would check for updates from server
  delay(60000);  // Check every minute

  // Optional: Send heartbeat to server
  if (WiFi.status() == WL_CONNECTED) {
    // TODO: Implement heartbeat to ESL Manager
    Serial.println("Heartbeat...");
  } else {
    // Try to reconnect
    Serial.println("WiFi disconnected, attempting reconnect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
  }
}
