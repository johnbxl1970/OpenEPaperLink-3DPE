/**
 * display_3dpe.cpp
 *
 * 3DPE Custom Startup Screen Implementation
 * Supports server-configurable display settings
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
void Display3DPE::init(int rotation) {
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
  display->setRotation(rotation);
  display->setTextColor(GxEPD_BLACK);
}

/**
 * Show the startup screen with device information (with config)
 */
void Display3DPE::showStartupScreen(const DeviceConfig& config) {
  init(config.display_rotation);

  // Show connecting message while fetching data
  showConnecting();

  // Get device MAC address
  String macAddress = WiFi.macAddress();

  // Fetch device info from server
  DeviceInfo info = fetchDeviceInfo(macAddress);

  // Render the startup screen with config
  renderStartupLayout(info, config);

  // Power down display to save energy
  display->hibernate();
}

/**
 * Show the startup screen with default configuration (backward compatibility)
 */
void Display3DPE::showStartupScreen() {
  showStartupScreen(DEFAULT_DEVICE_CONFIG);
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
      // Handle both direct properties and nested data object
      JsonObject data = doc.containsKey("data") ? doc["data"].as<JsonObject>() : doc.as<JsonObject>();

      // Extract device information
      if (data.containsKey("device_name")) {
        info.name = data["device_name"].as<String>();
        info.registered = true;
      }

      if (data.containsKey("metadata")) {
        JsonObject metadata = data["metadata"];

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
 * Render the complete startup screen layout with config
 */
void Display3DPE::renderStartupLayout(const DeviceInfo& info, const DeviceConfig& config) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // Draw 3DPE logo in top right corner (64x64 pixels) if enabled
    if (config.show_logo) {
      drawLogo(DISPLAY_WIDTH - 70, 6);
    }

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

/**
 * Update display with SmartPrinter status
 */
void Display3DPE::updateSmartPrinterStatus() {
  init(1);  // Landscape mode

  String macAddress = WiFi.macAddress();
  PrinterStatus status = fetchPrinterStatus(macAddress);
  renderSmartPrinterLayout(status);

  display->hibernate();
}

/**
 * Fetch printer status from ESL Manager server
 */
Display3DPE::PrinterStatus Display3DPE::fetchPrinterStatus(const String& macAddress) {
  PrinterStatus status;
  status.printerName = "Unregistered";
  status.status = "UNKNOWN";
  status.jobId = "--";
  status.orderNumber = "--";
  status.itemNumber = "--";
  status.boxId = "--";
  status.queueCount = 0;
  status.registered = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    status.status = "NO WIFI";
    return status;
  }

  // Fetch from SmartPrinter API endpoint
  String url = String(SERVER_URL) + "/api/smartprinter/mac/" + macAddress + "/data";
  Serial.printf("Fetching printer status from: %s\n", url.c_str());

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

      status.registered = true;

      if (data.containsKey("printer_name")) {
        status.printerName = data["printer_name"].as<String>();
      }
      if (data.containsKey("status")) {
        status.status = data["status"].as<String>();
        status.status.toUpperCase();
      }
      if (data.containsKey("job_id")) {
        String val = data["job_id"].as<String>();
        status.jobId = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("order_number")) {
        String val = data["order_number"].as<String>();
        status.orderNumber = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("item_number")) {
        String val = data["item_number"].as<String>();
        status.itemNumber = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("box_id")) {
        String val = data["box_id"].as<String>();
        status.boxId = (val == "0" || val == "" || val == "null") ? "--" : val;
      }
      if (data.containsKey("queue_count")) {
        status.queueCount = data["queue_count"].as<int>();
      }
    }
  } else if (httpCode == 404) {
    Serial.println("Device not registered or no printer assigned");
    status.status = "NOT ASSIGNED";
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    status.status = "ERROR";
  }

  http.end();
  return status;
}

/**
 * Render the SmartPrinter template layout (matches 296x128 template)
 */
void Display3DPE::renderSmartPrinterLayout(const PrinterStatus& status) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // === Header bar (black background) ===
    display->fillRect(0, 0, 296, 32, GxEPD_BLACK);

    // Printer name (white text on black)
    display->setFont(&FreeSans12pt7b);
    display->setTextColor(GxEPD_WHITE);
    display->setCursor(8, 22);
    // Truncate printer name if too long
    String printerName = status.printerName;
    if (printerName.length() > 14) {
      printerName = printerName.substring(0, 14);
    }
    display->print(printerName);

    // Status badge (white box with black text)
    display->fillRect(220, 6, 68, 20, GxEPD_WHITE);
    display->setFont(&FreeSans9pt7b);
    display->setTextColor(GxEPD_BLACK);
    // Center the status text
    int statusWidth = status.status.length() * 7;
    int statusX = 254 - (statusWidth / 2);
    display->setCursor(statusX, 20);
    display->print(status.status);

    // === Content area ===
    display->setTextColor(GxEPD_BLACK);

    // Job label and value
    display->setFont(NULL);  // Default font for labels
    display->setCursor(8, 40);
    display->print("Job");
    display->setFont(&FreeSans9pt7b);
    display->setCursor(8, 62);
    display->print(status.jobId);

    // Order label and value
    display->setFont(NULL);
    display->setCursor(150, 40);
    display->print("Order");
    display->setFont(&FreeSans9pt7b);
    display->setCursor(150, 62);
    display->print(status.orderNumber);

    // Separator line
    display->drawLine(8, 72, 288, 72, GxEPD_BLACK);

    // Item label and value
    display->setFont(NULL);
    display->setCursor(8, 80);
    display->print("Item");
    display->setFont(&FreeSans9pt7b);
    display->setCursor(8, 102);
    display->print(status.itemNumber);

    // Box ID label and value
    display->setFont(NULL);
    display->setCursor(150, 80);
    display->print("Box ID");
    display->setFont(&FreeSans9pt7b);
    display->setCursor(150, 102);
    display->print(status.boxId);

    // === Footer ===
    display->drawLine(0, 115, 296, 115, GxEPD_BLACK);
    display->setFont(NULL);
    display->setCursor(115, 120);
    display->print("SmartPrinter");

  } while (display->nextPage());
}
