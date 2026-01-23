/**
 * main_3dpe.cpp
 *
 * Simplified main file for 3DPE XIAO ESP32C3 ePaper display device
 * This is a standalone firmware that connects to WiFi and displays device info
 *
 * Features:
 * - Server-configurable settings via DeviceConfig
 * - Config version tracking for efficient updates
 * - Automatic config refresh when server version changes
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "display_3dpe.h"
#include "device_config.h"
#include "battery.h"

/**
 * Send heartbeat to ESL Manager server
 * Returns the config_version from the server response, or -1 on error
 */
int sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    return -1;
  }

  // Get battery status
  BatteryStatus battery = BatteryMonitor::getStatus();

  HTTPClient http;
  String url = String(SERVER_URL) + "/api/devices/heartbeat";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload with battery info
  String payload = "{";
  payload += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
  payload += "\"signal_strength\":" + String(WiFi.RSSI()) + ",";
  payload += "\"firmware_version\":\"1.2.0\",";
  payload += "\"battery_mv\":" + String(battery.voltage_mv) + ",";
  payload += "\"battery_percent\":" + String(battery.percentage) + ",";
  payload += "\"metadata\":{";
  payload += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  payload += "\"device_type\":\"" + String(DEVICE_TYPE) + "\",";
  payload += "\"config_version\":" + String(DeviceConfigManager::getConfigVersion()) + ",";
  payload += "\"battery_charging\":" + String(battery.is_charging ? "true" : "false");
  payload += "}}";

  int httpCode = http.POST(payload);
  int serverConfigVersion = -1;

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Heartbeat sent successfully");

    // Parse response to get config_version
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error && doc.containsKey("config_version")) {
      serverConfigVersion = doc["config_version"].as<int>();
      Serial.printf("Server config_version: %d (local: %d)\n",
                    serverConfigVersion,
                    DeviceConfigManager::getConfigVersion());
    }
  } else {
    Serial.printf("Heartbeat failed: %d\n", httpCode);
  }

  http.end();
  return serverConfigVersion;
}

/**
 * Check for config updates and refresh display if needed
 * @param serverConfigVersion Config version from heartbeat response
 * @return true if config was updated
 */
bool checkAndUpdateConfig(int serverConfigVersion) {
  if (serverConfigVersion < 0) {
    return false;
  }

  // Check if config version has changed
  if (DeviceConfigManager::needsRefresh(serverConfigVersion)) {
    Serial.println("Config version changed, fetching new config...");

    if (DeviceConfigManager::fetchConfig(WiFi.macAddress())) {
      Serial.println("Config updated successfully");
      DeviceConfigManager::printConfig();

      // Refresh SmartPrinter display with new config
      Serial.println("Refreshing SmartPrinter display with new config...");
      Display3DPE::updateSmartPrinterStatus();

      return true;
    } else {
      Serial.println("Failed to fetch updated config");
    }
  }

  return false;
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);  // Wait for serial to initialize

  Serial.println("\n\n=================================");
  Serial.println("3DPE Smart Manufacturing ESL");
  Serial.println("XIAO ESP32C3 + 2.9\" ePaper");
  Serial.println("Firmware v1.2.0 (Battery Monitor)");
  Serial.println("=================================");

  // Initialize config manager with defaults
  DeviceConfigManager::init();

  // Initialize battery monitor
  BatteryMonitor::init();
  BatteryMonitor::printStatus();

  // Print device info
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Device Type: ");
  Serial.println(DEVICE_TYPE);
  Serial.print("Server URL: ");
  Serial.println(SERVER_URL);

  // Get WiFi timeout from config (using default since not connected yet)
  DeviceConfig& config = DeviceConfigManager::getConfig();
  int wifiTimeout = config.wifi_timeout_seconds * 1000;

  // Try primary WiFi first
  Serial.printf("\nConnecting to primary WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < (unsigned long)wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // If primary fails, try backup WiFi
  #ifdef WIFI_SSID_BACKUP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Primary WiFi failed. Trying backup: %s\n", WIFI_SSID_BACKUP);
    WiFi.disconnect();
    delay(500);
    WiFi.begin(WIFI_SSID_BACKUP, WIFI_PASSWORD_BACKUP);

    startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < (unsigned long)wifiTimeout) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }
  #endif

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("Network: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Fetch config from server
    Serial.println("\nFetching config from server...");
    if (DeviceConfigManager::fetchConfig(WiFi.macAddress())) {
      DeviceConfigManager::printConfig();
    } else {
      Serial.println("Using default config");
    }
  } else {
    Serial.println("WiFi connection failed (both networks) - continuing in offline mode");
  }

  // Get updated config (may have changed after fetch)
  config = DeviceConfigManager::getConfig();

  // Display SmartPrinter status screen
  Serial.println("\nDisplaying SmartPrinter status...");
  Display3DPE::updateSmartPrinterStatus();
  Serial.println("SmartPrinter display complete");

  Serial.println("\n=================================");
  Serial.printf("Device ready - heartbeat every %d seconds\n", config.heartbeat_interval_seconds);
  Serial.println("=================================");
}

void loop() {
  // Get current config
  DeviceConfig& config = DeviceConfigManager::getConfig();

  // Wait for heartbeat interval
  delay(config.heartbeat_interval_seconds * 1000);

  if (WiFi.status() == WL_CONNECTED) {
    // Send heartbeat and get server config version
    int serverConfigVersion = sendHeartbeat();

    // Check if config needs update
    bool configUpdated = checkAndUpdateConfig(serverConfigVersion);

    // Refresh SmartPrinter display on heartbeat if enabled (and config wasn't just updated)
    if (!configUpdated && config.display_refresh_on_heartbeat) {
      Serial.println("Refreshing SmartPrinter display...");
      Display3DPE::updateSmartPrinterStatus();
    }
  } else {
    // Try to reconnect - try primary first, then backup
    Serial.println("WiFi disconnected, attempting reconnect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);

    #ifdef WIFI_SSID_BACKUP
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Primary failed, trying backup WiFi...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(WIFI_SSID_BACKUP, WIFI_PASSWORD_BACKUP);
      delay(5000);
    }
    #endif
  }
}
