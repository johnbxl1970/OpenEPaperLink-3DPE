/**
 * device_config.cpp
 *
 * Server-configurable device settings implementation
 */

#include "device_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Static member initialization
DeviceConfig DeviceConfigManager::currentConfig = DEFAULT_DEVICE_CONFIG;
bool DeviceConfigManager::configLoaded = false;

/**
 * Initialize the config manager with default values
 */
void DeviceConfigManager::init() {
  currentConfig = DEFAULT_DEVICE_CONFIG;
  configLoaded = false;
  Serial.println("[Config] Initialized with defaults");
}

/**
 * Fetch configuration from server
 */
bool DeviceConfigManager::fetchConfig(const String& macAddress) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Config] WiFi not connected, using defaults");
    return false;
  }

  // Build config endpoint URL
  String url = String(SERVER_URL) + "/api/devices/mac/" + macAddress + "/config";
  Serial.printf("[Config] Fetching from: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.printf("[Config] Response: %s\n", payload.c_str());

    if (parseConfigJson(payload)) {
      configLoaded = true;
      Serial.printf("[Config] Loaded successfully (version %d)\n", currentConfig.config_version);
      http.end();
      return true;
    } else {
      Serial.println("[Config] Failed to parse response");
    }
  } else if (httpCode == 404) {
    Serial.println("[Config] Device not registered, using defaults");
  } else {
    Serial.printf("[Config] HTTP error: %d\n", httpCode);
  }

  http.end();
  return false;
}

/**
 * Parse JSON response into DeviceConfig
 */
bool DeviceConfigManager::parseConfigJson(const String& json) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.printf("[Config] JSON parse error: %s\n", error.c_str());
    return false;
  }

  // Check for success flag
  if (!doc["success"].as<bool>()) {
    Serial.println("[Config] Server returned success=false");
    return false;
  }

  // Get config object
  JsonObject config = doc["config"];
  if (config.isNull()) {
    Serial.println("[Config] No config object in response");
    return false;
  }

  // Parse timing & behavior settings
  if (config.containsKey("heartbeat_interval_seconds")) {
    currentConfig.heartbeat_interval_seconds = config["heartbeat_interval_seconds"].as<int>();
  }
  if (config.containsKey("wifi_timeout_seconds")) {
    currentConfig.wifi_timeout_seconds = config["wifi_timeout_seconds"].as<int>();
  }
  if (config.containsKey("display_refresh_on_heartbeat")) {
    currentConfig.display_refresh_on_heartbeat = config["display_refresh_on_heartbeat"].as<bool>();
  }

  // Parse display settings
  if (config.containsKey("display_rotation")) {
    currentConfig.display_rotation = config["display_rotation"].as<int>();
  }
  if (config.containsKey("show_logo")) {
    currentConfig.show_logo = config["show_logo"].as<bool>();
  }
  if (config.containsKey("screen_brightness")) {
    currentConfig.screen_brightness = config["screen_brightness"].as<int>();
  }

  // Parse power management settings
  if (config.containsKey("deep_sleep_enabled")) {
    currentConfig.deep_sleep_enabled = config["deep_sleep_enabled"].as<bool>();
  }
  if (config.containsKey("wake_on_button")) {
    currentConfig.wake_on_button = config["wake_on_button"].as<bool>();
  }

  // Parse content settings
  if (config.containsKey("template_id")) {
    // template_id can be null, treat as 0
    if (config["template_id"].isNull()) {
      currentConfig.template_id = 0;
    } else {
      currentConfig.template_id = config["template_id"].as<int>();
    }
  }
  if (config.containsKey("content_refresh_seconds")) {
    currentConfig.content_refresh_seconds = config["content_refresh_seconds"].as<int>();
  }

  // Parse config version (top level, not inside config object)
  if (doc.containsKey("config_version")) {
    currentConfig.config_version = doc["config_version"].as<int>();
  }

  return true;
}

/**
 * Get the current device configuration
 */
DeviceConfig& DeviceConfigManager::getConfig() {
  return currentConfig;
}

/**
 * Check if config needs to be refreshed based on version
 */
bool DeviceConfigManager::needsRefresh(int serverVersion) {
  return serverVersion != currentConfig.config_version;
}

/**
 * Get the current config version
 */
int DeviceConfigManager::getConfigVersion() {
  return currentConfig.config_version;
}

/**
 * Print current configuration to Serial for debugging
 */
void DeviceConfigManager::printConfig() {
  Serial.println("[Config] Current configuration:");
  Serial.printf("  - heartbeat_interval_seconds: %d\n", currentConfig.heartbeat_interval_seconds);
  Serial.printf("  - wifi_timeout_seconds: %d\n", currentConfig.wifi_timeout_seconds);
  Serial.printf("  - display_refresh_on_heartbeat: %s\n", currentConfig.display_refresh_on_heartbeat ? "true" : "false");
  Serial.printf("  - display_rotation: %d\n", currentConfig.display_rotation);
  Serial.printf("  - show_logo: %s\n", currentConfig.show_logo ? "true" : "false");
  Serial.printf("  - screen_brightness: %d\n", currentConfig.screen_brightness);
  Serial.printf("  - deep_sleep_enabled: %s\n", currentConfig.deep_sleep_enabled ? "true" : "false");
  Serial.printf("  - wake_on_button: %s\n", currentConfig.wake_on_button ? "true" : "false");
  Serial.printf("  - template_id: %d\n", currentConfig.template_id);
  Serial.printf("  - content_refresh_seconds: %d\n", currentConfig.content_refresh_seconds);
  Serial.printf("  - config_version: %d\n", currentConfig.config_version);
}
