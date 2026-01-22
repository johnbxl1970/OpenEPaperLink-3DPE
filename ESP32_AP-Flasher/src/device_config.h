/**
 * device_config.h
 *
 * Server-configurable device settings for 3DPE devices
 * Enables dynamic control of device behavior without reflashing firmware
 */

#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

/**
 * Device configuration structure
 * All settings that can be controlled from the ESL Manager server
 */
struct DeviceConfig {
  // Timing & Behavior
  int heartbeat_interval_seconds;     // Time between server check-ins (default: 60)
  int wifi_timeout_seconds;           // WiFi connection timeout (default: 30)
  bool display_refresh_on_heartbeat;  // Refresh display on each heartbeat (default: false)

  // Display
  int display_rotation;               // Screen rotation 0-3 (default: 1)
  bool show_logo;                     // Show 3DPE logo on startup screen (default: true)
  int screen_brightness;              // Display contrast/brightness if supported (default: 100)

  // Power Management
  bool deep_sleep_enabled;            // Use deep sleep between heartbeats (default: false)
  bool wake_on_button;                // Allow button to wake from sleep (default: true)

  // Content
  int template_id;                    // Template to render on display (0 = none)
  int content_refresh_seconds;        // How often to fetch new content (default: 300)

  // Config versioning (for change detection)
  int config_version;                 // Server-tracked version number
};

// Default configuration values
const DeviceConfig DEFAULT_DEVICE_CONFIG = {
  // Timing & Behavior
  .heartbeat_interval_seconds = 60,
  .wifi_timeout_seconds = 30,
  .display_refresh_on_heartbeat = false,

  // Display
  .display_rotation = 1,
  .show_logo = true,
  .screen_brightness = 100,

  // Power Management
  .deep_sleep_enabled = false,
  .wake_on_button = true,

  // Content
  .template_id = 0,
  .content_refresh_seconds = 300,

  // Config versioning
  .config_version = 1
};

/**
 * DeviceConfigManager class
 * Handles fetching, parsing, and applying device configuration from server
 */
class DeviceConfigManager {
public:
  /**
   * Initialize the config manager
   */
  static void init();

  /**
   * Fetch configuration from server
   * @param macAddress Device MAC address
   * @return true if config was fetched successfully
   */
  static bool fetchConfig(const String& macAddress);

  /**
   * Get the current device configuration
   * @return Current DeviceConfig
   */
  static DeviceConfig& getConfig();

  /**
   * Check if config needs to be refreshed based on version
   * @param serverVersion Version number from heartbeat response
   * @return true if config version differs from server
   */
  static bool needsRefresh(int serverVersion);

  /**
   * Get the current config version
   * @return Current config version number
   */
  static int getConfigVersion();

  /**
   * Print current configuration to Serial for debugging
   */
  static void printConfig();

private:
  static DeviceConfig currentConfig;
  static bool configLoaded;

  /**
   * Parse JSON response into DeviceConfig
   * @param json JSON string from server
   * @return true if parsing was successful
   */
  static bool parseConfigJson(const String& json);
};

#endif // DEVICE_CONFIG_H
