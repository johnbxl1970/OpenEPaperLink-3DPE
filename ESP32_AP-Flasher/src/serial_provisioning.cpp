/**
 * Serial Provisioning Handler Implementation
 *
 * Processes JSON commands over serial for device configuration.
 */

#include "serial_provisioning.h"
#include "nvs_config.h"
#include <WiFi.h>

// Static member initialization
String SerialProvisioning::deviceType = "";
String SerialProvisioning::firmwareVersion = "";
char SerialProvisioning::serialBuffer[SERIAL_BUFFER_SIZE];
int SerialProvisioning::bufferIndex = 0;
bool SerialProvisioning::restartRequested = false;

void SerialProvisioning::init(const String& type, const String& version) {
    deviceType = type;
    firmwareVersion = version;
    bufferIndex = 0;
    restartRequested = false;

    Serial.println();
    Serial.println("===========================================");
    Serial.println("  SMD Serial Provisioning Mode");
    Serial.println("===========================================");
    Serial.printf("  Device Type: %s\n", deviceType.c_str());
    Serial.printf("  Firmware:    %s\n", firmwareVersion.c_str());
    Serial.printf("  MAC Address: %s\n", getMacAddress().c_str());
    Serial.println("===========================================");
    Serial.println("Awaiting configuration commands...");
    Serial.println();
}

bool SerialProvisioning::processSerial() {
    bool commandProcessed = false;

    while (Serial.available()) {
        char c = Serial.read();

        // End of line - process command
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                serialBuffer[bufferIndex] = '\0';
                executeCommand(serialBuffer);
                commandProcessed = true;
                bufferIndex = 0;
            }
        }
        // Add to buffer
        else if (bufferIndex < SERIAL_BUFFER_SIZE - 1) {
            serialBuffer[bufferIndex++] = c;
        }
        // Buffer overflow
        else {
            Serial.println("{\"status\":\"error\",\"msg\":\"command_too_long\"}");
            bufferIndex = 0;
        }
    }

    return commandProcessed;
}

bool SerialProvisioning::shouldRestart() {
    return restartRequested;
}

void SerialProvisioning::sendResponse(const char* status, const char* message) {
    JsonDocument doc;
    doc["status"] = status;
    doc["msg"] = message;
    serializeJson(doc, Serial);
    Serial.println();
}

void SerialProvisioning::sendDeviceInfo() {
    JsonDocument doc;
    doc["status"] = "ok";
    doc["mac"] = getMacAddress();
    doc["type"] = deviceType;
    doc["version"] = firmwareVersion;
    doc["provisioned"] = NVSConfig::isProvisioned();
    serializeJson(doc, Serial);
    Serial.println();
}

void SerialProvisioning::sendConfig() {
    JsonDocument doc;
    doc["status"] = "ok";

    JsonObject config = doc["config"].to<JsonObject>();
    config["provisioned"] = NVSConfig::isProvisioned();
    config["wifi_ssid"] = NVSConfig::getWiFiSSID();
    config["wifi_ssid_backup"] = NVSConfig::getWiFiSSIDBackup();
    config["server_url"] = NVSConfig::getServerURL();
    // Passwords not returned for security

    serializeJson(doc, Serial);
    Serial.println();
}

String SerialProvisioning::getMacAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(macStr);
}

void SerialProvisioning::executeCommand(const char* json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        sendResponse("error", "invalid_json");
        return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) {
        sendResponse("error", "missing_cmd");
        return;
    }

    // Command: get_info
    if (strcmp(cmd, "get_info") == 0) {
        sendDeviceInfo();
    }
    // Command: get_config
    else if (strcmp(cmd, "get_config") == 0) {
        sendConfig();
    }
    // Command: set_wifi
    else if (strcmp(cmd, "set_wifi") == 0) {
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        if (!ssid) {
            sendResponse("error", "missing_ssid");
            return;
        }

        NVSConfig::setWiFiSSID(ssid);
        if (password) {
            NVSConfig::setWiFiPassword(password);
        }
        sendResponse("ok", "wifi_set");
    }
    // Command: set_wifi_backup
    else if (strcmp(cmd, "set_wifi_backup") == 0) {
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        if (!ssid) {
            sendResponse("error", "missing_ssid");
            return;
        }

        NVSConfig::setWiFiSSIDBackup(ssid);
        if (password) {
            NVSConfig::setWiFiPasswordBackup(password);
        }
        sendResponse("ok", "wifi_backup_set");
    }
    // Command: set_server
    else if (strcmp(cmd, "set_server") == 0) {
        const char* url = doc["url"];

        if (!url) {
            sendResponse("error", "missing_url");
            return;
        }

        NVSConfig::setServerURL(url);
        sendResponse("ok", "server_set");
    }
    // Command: provision
    else if (strcmp(cmd, "provision") == 0) {
        // Validate that we have minimum required config
        String ssid = NVSConfig::getWiFiSSID();
        String serverUrl = NVSConfig::getServerURL();

        if (ssid.length() == 0) {
            sendResponse("error", "wifi_not_configured");
            return;
        }

        if (serverUrl.length() == 0) {
            sendResponse("error", "server_not_configured");
            return;
        }

        NVSConfig::setProvisioned(true);
        sendResponse("ok", "provisioned");

        // Schedule restart
        restartRequested = true;
    }
    // Command: reset
    else if (strcmp(cmd, "reset") == 0) {
        NVSConfig::clearAll();
        sendResponse("ok", "config_cleared");
        restartRequested = true;
    }
    // Command: reboot
    else if (strcmp(cmd, "reboot") == 0) {
        sendResponse("ok", "rebooting");
        restartRequested = true;
    }
    // Unknown command
    else {
        sendResponse("error", "unknown_command");
    }
}
