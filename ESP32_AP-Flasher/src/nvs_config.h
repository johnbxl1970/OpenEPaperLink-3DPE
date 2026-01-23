/**
 * NVS Configuration Manager
 *
 * Stores and retrieves device configuration from ESP32 non-volatile storage.
 * Used for serial-based provisioning of WiFi credentials and server URL.
 */

#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// NVS Namespace for SMD configuration
#define NVS_NAMESPACE "smd_config"

// Maximum string lengths
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_URL_LENGTH 256

class NVSConfig {
public:
    /**
     * Initialize NVS configuration
     * Must be called in setup() before any other NVS operations
     */
    static void init();

    /**
     * Check if device has been provisioned
     * @return true if device has valid WiFi and server configuration
     */
    static bool isProvisioned();

    /**
     * Set provisioned flag
     * @param value true to mark device as provisioned
     */
    static void setProvisioned(bool value);

    // Primary WiFi credentials
    static String getWiFiSSID();
    static void setWiFiSSID(const String& ssid);
    static String getWiFiPassword();
    static void setWiFiPassword(const String& password);

    // Backup WiFi credentials (optional)
    static String getWiFiSSIDBackup();
    static void setWiFiSSIDBackup(const String& ssid);
    static String getWiFiPasswordBackup();
    static void setWiFiPasswordBackup(const String& password);

    // Server configuration
    static String getServerURL();
    static void setServerURL(const String& url);

    /**
     * Clear all stored configuration
     * Resets device to un-provisioned state
     */
    static void clearAll();

    /**
     * Get configuration summary for debugging
     * @return JSON string with configuration (passwords masked)
     */
    static String getConfigSummary();

private:
    static Preferences preferences;
    static bool initialized;

    // Ensure preferences are open for read/write
    static void ensureOpen(bool readOnly = false);
    static void close();
};

#endif // NVS_CONFIG_H
