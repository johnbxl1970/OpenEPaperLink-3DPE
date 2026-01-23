/**
 * main_smd.cpp
 *
 * Unified Smart Manufacturing Display (SMD) Firmware
 * Works with any display size via the IDisplay interface
 *
 * Features:
 * - Server-configurable settings via DeviceConfig
 * - Config version tracking for efficient updates
 * - Automatic config refresh when server version changes
 * - Battery monitoring (if supported)
 * - Backup WiFi support
 *
 * The display driver is selected at compile time via DISPLAY_TYPE flag:
 * - DISPLAY_TYPE_2INCH9: 2.9" ePaper (296x128)
 * - DISPLAY_TYPE_4INCH2: 4.2" ePaper (400x300)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "display_interface.h"
#include "device_config.h"
#include "nvs_config.h"
#include "serial_provisioning.h"

// Include battery monitor if available
#ifdef HAS_BATTERY_MONITOR
#include "battery.h"
#endif

// Flag to indicate if this is a generic (provisioning-capable) build
// Generic builds do NOT define USE_COMPILE_TIME_CREDENTIALS
#ifndef USE_COMPILE_TIME_CREDENTIALS
#define USE_NVS_PROVISIONING 1
#endif

// Firmware version
#define FIRMWARE_VERSION "2.0.0"

// Device type string for provisioning
#if defined(DISPLAY_TYPE_2INCH9)
#define DEVICE_TYPE_STRING "SMD_2inch9"
#elif defined(DISPLAY_TYPE_4INCH2)
#define DEVICE_TYPE_STRING "SMD_4inch2"
#else
#define DEVICE_TYPE_STRING "SMD_Unknown"
#endif

// Configuration from build flags (for non-provisioning builds)
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef WIFI_SSID_BACKUP
#define WIFI_SSID_BACKUP ""
#endif

#ifndef WIFI_PASSWORD_BACKUP
#define WIFI_PASSWORD_BACKUP ""
#endif

#ifndef SERVER_URL
#define SERVER_URL "http://192.168.1.100:3001"
#endif

// Runtime configuration (loaded from NVS or compile-time)
static String runtimeSSID;
static String runtimePassword;
static String runtimeSSIDBackup;
static String runtimePasswordBackup;
static String runtimeServerURL;

/**
 * Send heartbeat to ESL Manager server
 * Returns the config_version from the server response, or -1 on error
 */
int sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    return -1;
  }

  HTTPClient http;
  String url = runtimeServerURL + "/api/devices/heartbeat";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Get display info
  DisplayInfo displayInfo = g_display->getDisplayInfo();

  // Build JSON payload
  String payload = "{";
  payload += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
  payload += "\"signal_strength\":" + String(WiFi.RSSI()) + ",";
  payload += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";

  // Battery info if available
  #ifdef HAS_BATTERY_MONITOR
  BatteryStatus battery = BatteryMonitor::getStatus();
  payload += "\"battery_mv\":" + String(battery.voltage_mv) + ",";
  payload += "\"battery_percent\":" + String(battery.percentage) + ",";
  #endif

  payload += "\"metadata\":{";
  payload += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  payload += "\"device_type\":\"smd\",";
  payload += "\"display_type\":\"" + String(displayInfo.type) + "\",";
  payload += "\"display_width\":" + String(displayInfo.width) + ",";
  payload += "\"display_height\":" + String(displayInfo.height) + ",";
  payload += "\"config_version\":" + String(DeviceConfigManager::getConfigVersion());

  #ifdef HAS_BATTERY_MONITOR
  payload += ",\"battery_charging\":" + String(battery.is_charging ? "true" : "false");
  #endif

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
 * Fetch display content from server
 */
DisplayContent fetchContent() {
  DisplayContent content;
  content.title = "Unregistered";
  content.subtitle = "SMD";
  content.status = "UNKNOWN";
  content.line1 = "--";
  content.line2 = "--";
  content.line3 = "--";
  content.line4 = "--";
  content.footerLeft = WiFi.macAddress();
  content.footerRight = String(g_display->getDisplayInfo().width) + "x" +
                        String(g_display->getDisplayInfo().height);
  content.showLogo = true;
  content.batteryPercent = -1;
  content.signalStrength = WiFi.RSSI();

  #ifdef HAS_BATTERY_MONITOR
  BatteryStatus battery = BatteryMonitor::getStatus();
  if (battery.is_valid) {
    content.batteryPercent = battery.percentage;
  }
  #endif

  if (WiFi.status() != WL_CONNECTED) {
    content.status = "NO WIFI";
    return content;
  }

  // Fetch from SmartPrinter API endpoint (server sends content based on assignment)
  String url = runtimeServerURL + "/api/smartprinter/mac/" + WiFi.macAddress() + "/data";
  Serial.printf("Fetching content from: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.printf("Response: %s\n", payload.c_str());

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Check for nested data object
      JsonObject data = doc.containsKey("data") ? doc["data"].as<JsonObject>() : doc.as<JsonObject>();

      // Title - use printer_name or device_name
      if (data.containsKey("printer_name")) {
        content.title = data["printer_name"].as<String>();
      } else if (data.containsKey("device_name")) {
        content.title = data["device_name"].as<String>();
      }

      // Status
      if (data.containsKey("status")) {
        content.status = data["status"].as<String>();
        content.status.toUpperCase();
      }

      // Content fields - map from API response
      if (data.containsKey("job_id")) {
        String val = data["job_id"].as<String>();
        content.line1 = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("order_number")) {
        String val = data["order_number"].as<String>();
        content.line2 = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("item_number")) {
        String val = data["item_number"].as<String>();
        content.line3 = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("box_id")) {
        String val = data["box_id"].as<String>();
        content.line4 = (val == "0" || val == "" || val == "null") ? "--" : val;
      }

      // Subtitle from content_type if available
      if (data.containsKey("content_type")) {
        content.subtitle = data["content_type"].as<String>();
      }
    }
  } else if (httpCode == 404) {
    Serial.println("Device not registered or not assigned");
    content.status = "NOT ASSIGNED";
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    content.status = "ERROR";
  }

  http.end();
  return content;
}

/**
 * Check for config updates and refresh display if needed
 */
bool checkAndUpdateConfig(int serverConfigVersion) {
  if (serverConfigVersion < 0) {
    return false;
  }

  if (DeviceConfigManager::needsRefresh(serverConfigVersion)) {
    Serial.println("Config version changed, fetching new config...");

    if (DeviceConfigManager::fetchConfig(WiFi.macAddress())) {
      Serial.println("Config updated successfully");
      DeviceConfigManager::printConfig();

      // Refresh display with new config
      DisplayContent content = fetchContent();
      g_display->renderContent(content);
      return true;
    } else {
      Serial.println("Failed to fetch updated config");
    }
  }

  return false;
}

/**
 * Connect to WiFi with backup network support
 * Uses runtime credentials (loaded from NVS or compile-time)
 */
bool connectToWiFi(int timeout_ms) {
  Serial.printf("Connecting to WiFi: %s\n", runtimeSSID.c_str());
  g_display->showConnecting(runtimeSSID.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(runtimeSSID.c_str(), runtimePassword.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < (unsigned long)timeout_ms) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Try backup WiFi if primary fails and backup is configured
  if (WiFi.status() != WL_CONNECTED && runtimeSSIDBackup.length() > 0) {
    Serial.printf("Primary WiFi failed. Trying backup: %s\n", runtimeSSIDBackup.c_str());
    g_display->showConnecting(runtimeSSIDBackup.c_str());

    WiFi.disconnect();
    delay(500);
    WiFi.begin(runtimeSSIDBackup.c_str(), runtimePasswordBackup.c_str());

    startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < (unsigned long)timeout_ms) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }

  return WiFi.status() == WL_CONNECTED;
}

/**
 * Load runtime configuration from NVS or compile-time defines
 */
void loadRuntimeConfig() {
#ifdef USE_NVS_PROVISIONING
  // Load from NVS
  runtimeSSID = NVSConfig::getWiFiSSID();
  runtimePassword = NVSConfig::getWiFiPassword();
  runtimeSSIDBackup = NVSConfig::getWiFiSSIDBackup();
  runtimePasswordBackup = NVSConfig::getWiFiPasswordBackup();
  runtimeServerURL = NVSConfig::getServerURL();
#else
  // Use compile-time values
  runtimeSSID = WIFI_SSID;
  runtimePassword = WIFI_PASSWORD;
  runtimeSSIDBackup = WIFI_SSID_BACKUP;
  runtimePasswordBackup = WIFI_PASSWORD_BACKUP;
  runtimeServerURL = SERVER_URL;
#endif
}

/**
 * Enter serial provisioning mode
 * Waits for configuration commands over serial
 */
void enterProvisioningMode() {
  // Initialize serial provisioning
  SerialProvisioning::init(DEVICE_TYPE_STRING, FIRMWARE_VERSION);

  // Show provisioning screen on display
  DisplayContent content;
  content.title = "Setup Required";
  content.subtitle = "Provisioning";
  content.status = "AWAITING CONFIG";
  content.line1 = "Connect via USB";
  content.line2 = "Send WiFi credentials";
  content.line3 = "and server URL";
  content.line4 = "";
  content.footerLeft = WiFi.macAddress();
  content.footerRight = FIRMWARE_VERSION;
  content.showLogo = true;
  content.batteryPercent = -1;
  content.signalStrength = 0;

  #ifdef HAS_BATTERY_MONITOR
  BatteryStatus battery = BatteryMonitor::getStatus();
  if (battery.is_valid) {
    content.batteryPercent = battery.percentage;
  }
  #endif

  g_display->renderContent(content);

  Serial.println("Entering provisioning mode...");
  Serial.println("Send JSON commands to configure device.");
  Serial.println("Example: {\"cmd\":\"get_info\"}");

  // Process serial commands until provisioned
  while (!SerialProvisioning::shouldRestart()) {
    SerialProvisioning::processSerial();
    delay(50); // Small delay to prevent CPU spin
  }

  // Restart after provisioning
  Serial.println("Provisioning complete. Restarting...");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize NVS config manager
  NVSConfig::init();

  // Get display info
  DisplayInfo displayInfo = g_display->getDisplayInfo();

  Serial.println("\n\n=================================");
  Serial.println("Smart Manufacturing Display (SMD)");
  Serial.printf("Display: %s (%dx%d)\n", displayInfo.type, displayInfo.width, displayInfo.height);
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.println("=================================");

  // Initialize config manager
  DeviceConfigManager::init();

  // Initialize battery monitor if available
  #ifdef HAS_BATTERY_MONITOR
  BatteryMonitor::init();
  BatteryMonitor::printStatus();
  #endif

  // Print device info
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());

#ifdef USE_NVS_PROVISIONING
  // Check if device is provisioned
  if (!NVSConfig::isProvisioned()) {
    Serial.println("Device not provisioned - entering setup mode");
    enterProvisioningMode();
    // enterProvisioningMode() restarts the device after provisioning
    return;
  }
  Serial.println("Device is provisioned");
#endif

  // Load runtime configuration (from NVS or compile-time)
  loadRuntimeConfig();

  Serial.printf("Server URL: %s\n", runtimeServerURL.c_str());

  // Get WiFi timeout from config
  DeviceConfig& config = DeviceConfigManager::getConfig();
  int wifiTimeout = config.wifi_timeout_seconds * 1000;

  // Connect to WiFi
  if (connectToWiFi(wifiTimeout)) {
    Serial.println("WiFi Connected!");
    Serial.printf("Network: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());

    // Fetch config from server
    Serial.println("\nFetching config from server...");
    if (DeviceConfigManager::fetchConfig(WiFi.macAddress(), runtimeServerURL)) {
      DeviceConfigManager::printConfig();
    } else {
      Serial.println("Using default config");
    }
  } else {
    Serial.println("WiFi connection failed - continuing in offline mode");
    g_display->showError("WiFi Failed", "Check network credentials");
    delay(3000);
  }

  // Update config reference
  config = DeviceConfigManager::getConfig();

  // Show startup screen or content
  if (WiFi.status() == WL_CONNECTED) {
    DisplayContent content = fetchContent();

    if (content.status == "NOT ASSIGNED") {
      g_display->showWaitingForAssignment();
    } else {
      g_display->renderContent(content);
    }
  } else {
    g_display->showStartupScreen(WiFi.macAddress(), FIRMWARE_VERSION, runtimeServerURL.c_str());
  }

  Serial.println("\n=================================");
  Serial.printf("Device ready - heartbeat every %d seconds\n", config.heartbeat_interval_seconds);
  Serial.println("=================================");
}

void loop() {
  DeviceConfig& config = DeviceConfigManager::getConfig();

  // Wait for heartbeat interval
  delay(config.heartbeat_interval_seconds * 1000);

  if (WiFi.status() == WL_CONNECTED) {
    // Send heartbeat and check for config updates
    int serverConfigVersion = sendHeartbeat();
    bool configUpdated = checkAndUpdateConfig(serverConfigVersion);

    // Refresh display on heartbeat if enabled (and config wasn't just updated)
    if (!configUpdated && config.display_refresh_on_heartbeat) {
      Serial.println("Refreshing display...");
      DisplayContent content = fetchContent();
      g_display->renderContent(content);
    }
  } else {
    // Try to reconnect
    Serial.println("WiFi disconnected, attempting reconnect...");

    DeviceConfig& config = DeviceConfigManager::getConfig();
    if (connectToWiFi(config.wifi_timeout_seconds * 1000)) {
      Serial.println("WiFi reconnected!");
      DisplayContent content = fetchContent();
      g_display->renderContent(content);
    }
  }
}
