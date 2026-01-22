/**
 * display_3dpe.cpp
 *
 * 3DPE Custom Startup Screen Implementation
 *
 * Place this file in: ESP32_AP-Flasher/src/display_3dpe.cpp
 */

#include "display_3dpe.h"
#include "3dpe_logo.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

// Initialize static display pointer
GxEPD2_BW<DISPLAY_CLASS, DISPLAY_CLASS::HEIGHT>* Display3DPE::display = nullptr;

/**
 * Initialize the ePaper display
 */
void Display3DPE::init() {
  if (!display) {
    // Short delay to let hardware stabilize
    delay(100);

    // Initialize SPI with XIAO ESP32C3 default pins
    SPI.begin();

    display = new GxEPD2_BW<DISPLAY_CLASS, DISPLAY_CLASS::HEIGHT>(
      DISPLAY_CLASS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
    );
  }

  // Initialize display with longer reset duration for better compatibility
  display->init(115200, true, 10, false);  // serial, initial=true, reset_duration=10ms, pulldown_rst=false
  delay(100);
  display->setRotation(1);  // Landscape orientation
  display->setTextColor(GxEPD_BLACK);
}

/**
 * Show the startup screen with device information
 */
void Display3DPE::showStartupScreen() {
  init();

  // Show connecting message while fetching data
  showConnecting();

  // Get device MAC address
  String macAddress = WiFi.macAddress();

  // Fetch device info from server
  DeviceInfo info = fetchDeviceInfo(macAddress);

  // Render the startup screen
  renderStartupLayout(info);

  // Power down display to save energy
  display->hibernate();
}

/**
 * Fetch device configuration from ESL Manager server
 */
Display3DPE::DeviceInfo Display3DPE::fetchDeviceInfo(const String& macAddress) {
  DeviceInfo info;
  info.name = "Unregistered";
  info.deviceType = DEVICE_TYPE;
  info.commType = "wifi";
  info.registered = false;

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, using defaults");
    return info;
  }

  // Build API URL
  String url = String(SERVER_URL) + "/api/devices/mac/" + macAddress;
  Serial.printf("Fetching device info from: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(5000);  // 5 second timeout

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.printf("Response: %s\n", payload.c_str());

    // Parse JSON response
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Extract device information
      if (doc.containsKey("device_name")) {
        info.name = doc["device_name"].as<String>();
        info.registered = true;
      }

      if (doc.containsKey("metadata")) {
        JsonObject metadata = doc["metadata"];

        if (metadata.containsKey("device_type")) {
          info.deviceType = metadata["device_type"].as<String>();
        }

        if (metadata.containsKey("communication_type")) {
          info.commType = metadata["communication_type"].as<String>();
        }
      }

      Serial.printf("Device registered: %s (%s)\n", info.name.c_str(), info.deviceType.c_str());
    } else {
      Serial.printf("JSON parse error: %s\n", error.c_str());
    }
  } else if (httpCode == 404) {
    Serial.println("Device not registered on server (404)");
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
  }

  http.end();
  return info;
}

/**
 * Render the complete startup screen layout
 */
void Display3DPE::renderStartupLayout(const DeviceInfo& info) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // Draw 3DPE logo in top right corner (64x64 pixels)
    drawLogo(DISPLAY_WIDTH - 70, 6);

    // Draw device information - left side
    display->setFont(&FreeSans9pt7b);

    int yPos = 25;
    int lineSpacing = 20;

    // MAC Address
    display->setCursor(10, yPos);
    display->print("MAC: ");
    display->setFont(&FreeSans9pt7b);
    display->print(formatMacAddress(WiFi.macAddress()));
    yPos += lineSpacing;

    // Device Name
    display->setCursor(10, yPos);
    display->print("Name: ");
    display->print(info.name);
    yPos += lineSpacing;

    // Device Type
    display->setCursor(10, yPos);
    display->print("Type: ");
    display->print(info.deviceType);
    yPos += lineSpacing;

    // Communication Type
    display->setCursor(10, yPos);
    display->print("Comm: ");
    display->print(info.commType);
    yPos += lineSpacing;

    // WiFi SSID (if connected)
    if (WiFi.status() == WL_CONNECTED) {
      display->setCursor(10, yPos);
      display->print("WiFi: ");
      display->print(WiFi.SSID());
      yPos += lineSpacing;
    }

    // Screen Resolution - bottom right
    display->setFont(&FreeSans9pt7b);
    display->setCursor(DISPLAY_WIDTH - 80, DISPLAY_HEIGHT - 10);
    display->print(String(DISPLAY_WIDTH) + "x" + String(DISPLAY_HEIGHT));

    // Registration status indicator
    if (!info.registered) {
      display->drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);
    }

  } while (display->nextPage());
}

/**
 * Draw 3DPE logo in top right corner
 */
void Display3DPE::drawLogo(int x, int y) {
  // Draw logo bitmap (64x64 pixels)
  display->drawBitmap(x, y, logo_3dpe_64x64, LOGO_3DPE_WIDTH, LOGO_3DPE_HEIGHT, GxEPD_BLACK);
}

/**
 * Format MAC address for display
 */
String Display3DPE::formatMacAddress(const String& mac) {
  // Convert to uppercase and ensure standard format
  String formatted = mac;
  formatted.toUpperCase();
  return formatted;
}

/**
 * Show error message on display
 */
void Display3DPE::showError(const String& message) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // Draw error border
    display->drawRect(5, 5, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 10, GxEPD_BLACK);
    display->drawRect(6, 6, DISPLAY_WIDTH - 12, DISPLAY_HEIGHT - 12, GxEPD_BLACK);

    // Display error message
    display->setFont(&FreeSans12pt7b);
    display->setCursor(20, 60);
    display->print("ERROR");

    display->setFont(&FreeSans9pt7b);
    display->setCursor(20, 90);
    display->print(message);

  } while (display->nextPage());
}

/**
 * Clear display and show "Connecting..." message
 */
void Display3DPE::showConnecting() {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    display->setFont(&FreeSans12pt7b);
    display->setCursor(60, 64);
    display->print("Connecting...");

  } while (display->nextPage());
}
