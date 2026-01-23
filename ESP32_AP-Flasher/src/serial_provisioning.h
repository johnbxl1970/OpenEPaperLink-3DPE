/**
 * Serial Provisioning Handler
 *
 * JSON-based serial protocol for configuring WiFi credentials and server URL.
 * Used during device setup before WiFi is configured.
 *
 * Protocol Commands (from host to device):
 *   {"cmd": "get_info"}                              - Get device info (MAC, type, version)
 *   {"cmd": "get_config"}                            - Get current configuration
 *   {"cmd": "set_wifi", "ssid": "...", "password": "..."} - Set primary WiFi
 *   {"cmd": "set_wifi_backup", "ssid": "...", "password": "..."} - Set backup WiFi
 *   {"cmd": "set_server", "url": "http://..."}       - Set server URL
 *   {"cmd": "provision"}                             - Mark device as provisioned
 *   {"cmd": "reset"}                                 - Clear all config and restart
 *   {"cmd": "reboot"}                                - Restart device
 *
 * Protocol Responses (from device to host):
 *   {"status": "ok", "mac": "...", "type": "...", "version": "..."} - For get_info
 *   {"status": "ok", "config": {...}}                - For get_config
 *   {"status": "ok", "msg": "..."}                   - For successful operations
 *   {"status": "error", "msg": "..."}                - For errors
 */

#ifndef SERIAL_PROVISIONING_H
#define SERIAL_PROVISIONING_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Buffer sizes
#define SERIAL_BUFFER_SIZE 512
#define JSON_DOC_SIZE 1024

// Timeout for reading a complete command (ms)
#define SERIAL_READ_TIMEOUT 100

class SerialProvisioning {
public:
    /**
     * Initialize serial provisioning
     * @param deviceType Device type string (e.g., "SMD_2inch9")
     * @param firmwareVersion Firmware version string
     */
    static void init(const String& deviceType, const String& firmwareVersion);

    /**
     * Process incoming serial data
     * Call this repeatedly in loop() during provisioning mode
     * @return true if a command was processed
     */
    static bool processSerial();

    /**
     * Check if provisioning is complete and device should restart
     * @return true if provision command was received
     */
    static bool shouldRestart();

    /**
     * Send a response message
     * @param status "ok" or "error"
     * @param message Response message
     */
    static void sendResponse(const char* status, const char* message);

    /**
     * Send device info response
     */
    static void sendDeviceInfo();

    /**
     * Send current configuration
     */
    static void sendConfig();

private:
    static String deviceType;
    static String firmwareVersion;
    static char serialBuffer[SERIAL_BUFFER_SIZE];
    static int bufferIndex;
    static bool restartRequested;

    /**
     * Parse and execute a command
     * @param json JSON command string
     */
    static void executeCommand(const char* json);

    /**
     * Get device MAC address
     * @return MAC address string (XX:XX:XX:XX:XX:XX)
     */
    static String getMacAddress();
};

#endif // SERIAL_PROVISIONING_H
