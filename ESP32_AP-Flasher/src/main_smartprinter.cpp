/**
 * main_smartprinter.cpp
 *
 * SmartPrinter Firmware for XIAO ESP32C3
 * 4.2" Monochrome ePaper Display (400x300 pixels)
 *
 * This firmware:
 * - Connects to WiFi
 * - Fetches printer status from ESL Manager
 * - Displays printer info on ePaper display
 * - Periodically updates the display
 */

#include <Arduino.h>
#include <WiFi.h>
#include "display_smartprinter.h"

// Configuration from build flags
#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif

#ifndef SERVER_URL
#define SERVER_URL "http://192.168.1.100:3001"
#endif

#ifndef POLL_INTERVAL
#define POLL_INTERVAL 30000  // 30 seconds
#endif

// Global variables
unsigned long lastUpdate = 0;
bool wifiConnected = false;

/**
 * Connect to WiFi network
 */
bool connectToWiFi() {
  Serial.println("\n=================================");
  Serial.println("SmartPrinter Firmware v1.0");
  Serial.println("=================================");
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("Server URL: %s\n", SERVER_URL);
  Serial.println();

  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
}

/**
 * Setup function - runs once on boot
 */
void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for serial to initialize

  // Connect to WiFi
  wifiConnected = connectToWiFi();

  // Show startup screen
  Serial.println("\nDisplaying startup screen...");
  DisplaySmartPrinter::showStartupScreen();
  Serial.println("Startup screen complete");

  lastUpdate = millis();
}

/**
 * Main loop - runs continuously
 */
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("WiFi disconnected, attempting reconnect...");
      wifiConnected = false;
    }

    // Try to reconnect
    WiFi.reconnect();
    delay(5000);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected!");
      wifiConnected = true;
      // Update display after reconnection
      DisplaySmartPrinter::updateStatus();
      lastUpdate = millis();
    }
    return;
  }

  // Periodic status update
  if (millis() - lastUpdate >= POLL_INTERVAL) {
    Serial.println("\nUpdating display...");
    DisplaySmartPrinter::updateStatus();
    lastUpdate = millis();
    Serial.println("Display updated");
  }

  delay(1000);  // Check every second
}
