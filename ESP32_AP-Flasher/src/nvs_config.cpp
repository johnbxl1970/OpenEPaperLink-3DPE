/**
 * NVS Configuration Manager Implementation
 *
 * Uses ESP32 Preferences library for persistent storage of device configuration.
 */

#include "nvs_config.h"

// Static member initialization
Preferences NVSConfig::preferences;
bool NVSConfig::initialized = false;

// NVS Keys
static const char* KEY_PROVISIONED = "provisioned";
static const char* KEY_WIFI_SSID = "wifi_ssid";
static const char* KEY_WIFI_PASS = "wifi_pass";
static const char* KEY_WIFI_SSID_BK = "wifi_ssid_bk";
static const char* KEY_WIFI_PASS_BK = "wifi_pass_bk";
static const char* KEY_SERVER_URL = "server_url";

void NVSConfig::init() {
    if (initialized) return;

    // Just mark as initialized - we open/close for each operation
    // to avoid holding the NVS open
    initialized = true;
    Serial.println("[NVS] Configuration manager initialized");
}

void NVSConfig::ensureOpen(bool readOnly) {
    if (!initialized) {
        init();
    }
    preferences.begin(NVS_NAMESPACE, readOnly);
}

void NVSConfig::close() {
    preferences.end();
}

bool NVSConfig::isProvisioned() {
    ensureOpen(true);
    bool result = preferences.getBool(KEY_PROVISIONED, false);
    close();
    return result;
}

void NVSConfig::setProvisioned(bool value) {
    ensureOpen(false);
    preferences.putBool(KEY_PROVISIONED, value);
    close();
    Serial.printf("[NVS] Provisioned set to: %s\n", value ? "true" : "false");
}

String NVSConfig::getWiFiSSID() {
    ensureOpen(true);
    String result = preferences.getString(KEY_WIFI_SSID, "");
    close();
    return result;
}

void NVSConfig::setWiFiSSID(const String& ssid) {
    if (ssid.length() > MAX_SSID_LENGTH) {
        Serial.println("[NVS] Error: SSID too long");
        return;
    }
    ensureOpen(false);
    preferences.putString(KEY_WIFI_SSID, ssid);
    close();
    Serial.printf("[NVS] WiFi SSID set to: %s\n", ssid.c_str());
}

String NVSConfig::getWiFiPassword() {
    ensureOpen(true);
    String result = preferences.getString(KEY_WIFI_PASS, "");
    close();
    return result;
}

void NVSConfig::setWiFiPassword(const String& password) {
    if (password.length() > MAX_PASSWORD_LENGTH) {
        Serial.println("[NVS] Error: Password too long");
        return;
    }
    ensureOpen(false);
    preferences.putString(KEY_WIFI_PASS, password);
    close();
    Serial.println("[NVS] WiFi password set");
}

String NVSConfig::getWiFiSSIDBackup() {
    ensureOpen(true);
    String result = preferences.getString(KEY_WIFI_SSID_BK, "");
    close();
    return result;
}

void NVSConfig::setWiFiSSIDBackup(const String& ssid) {
    if (ssid.length() > MAX_SSID_LENGTH) {
        Serial.println("[NVS] Error: Backup SSID too long");
        return;
    }
    ensureOpen(false);
    preferences.putString(KEY_WIFI_SSID_BK, ssid);
    close();
    Serial.printf("[NVS] Backup WiFi SSID set to: %s\n", ssid.c_str());
}

String NVSConfig::getWiFiPasswordBackup() {
    ensureOpen(true);
    String result = preferences.getString(KEY_WIFI_PASS_BK, "");
    close();
    return result;
}

void NVSConfig::setWiFiPasswordBackup(const String& password) {
    if (password.length() > MAX_PASSWORD_LENGTH) {
        Serial.println("[NVS] Error: Backup password too long");
        return;
    }
    ensureOpen(false);
    preferences.putString(KEY_WIFI_PASS_BK, password);
    close();
    Serial.println("[NVS] Backup WiFi password set");
}

String NVSConfig::getServerURL() {
    ensureOpen(true);
    String result = preferences.getString(KEY_SERVER_URL, "");
    close();
    return result;
}

void NVSConfig::setServerURL(const String& url) {
    if (url.length() > MAX_URL_LENGTH) {
        Serial.println("[NVS] Error: Server URL too long");
        return;
    }
    ensureOpen(false);
    preferences.putString(KEY_SERVER_URL, url);
    close();
    Serial.printf("[NVS] Server URL set to: %s\n", url.c_str());
}

void NVSConfig::clearAll() {
    ensureOpen(false);
    preferences.clear();
    close();
    Serial.println("[NVS] All configuration cleared");
}

String NVSConfig::getConfigSummary() {
    ensureOpen(true);

    String ssid = preferences.getString(KEY_WIFI_SSID, "");
    String ssidBackup = preferences.getString(KEY_WIFI_SSID_BK, "");
    String serverUrl = preferences.getString(KEY_SERVER_URL, "");
    bool provisioned = preferences.getBool(KEY_PROVISIONED, false);

    close();

    // Build JSON summary (passwords masked)
    String json = "{";
    json += "\"provisioned\":" + String(provisioned ? "true" : "false") + ",";
    json += "\"wifi_ssid\":\"" + ssid + "\",";
    json += "\"wifi_password\":\"" + (preferences.getString(KEY_WIFI_PASS, "").length() > 0 ? "****" : "") + "\",";
    json += "\"wifi_ssid_backup\":\"" + ssidBackup + "\",";
    json += "\"wifi_password_backup\":\"" + (preferences.getString(KEY_WIFI_PASS_BK, "").length() > 0 ? "****" : "") + "\",";
    json += "\"server_url\":\"" + serverUrl + "\"";
    json += "}";

    return json;
}
