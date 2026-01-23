/**
 * display_smartprinter.cpp
 *
 * SmartPrinter Display Implementation
 * 4.2" Monochrome ePaper Display (400x300 pixels)
 *
 * Displays:
 * - Printer name and status
 * - Current job ID
 * - Order number and item number
 * - Box ID
 * - 3DPE branding
 *
 * Place this file in: ESP32_AP-Flasher/src/display_smartprinter.cpp
 */

#include "display_smartprinter.h"
#include "3dpe_logo.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// Initialize static display pointer
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>* DisplaySmartPrinter::display = nullptr;

/**
 * Initialize the ePaper display
 */
void DisplaySmartPrinter::init() {
  if (!display) {
    display = new GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>(
      GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
    );
  }

  display->init(115200);
  display->setRotation(0);  // Portrait orientation (400 wide x 300 tall)
  display->setTextColor(GxEPD_BLACK);
}

/**
 * Show the startup screen
 */
void DisplaySmartPrinter::showStartupScreen() {
  init();
  showConnecting();

  // Get device MAC address
  String macAddress = WiFi.macAddress();

  // Fetch printer status from server
  PrinterStatus status = fetchPrinterStatus(macAddress);

  // Render the display
  renderLayout(status);

  // Power down display to save energy
  display->hibernate();
}

/**
 * Update display with current status
 */
void DisplaySmartPrinter::updateStatus() {
  init();

  String macAddress = WiFi.macAddress();
  PrinterStatus status = fetchPrinterStatus(macAddress);
  renderLayout(status);

  display->hibernate();
}

/**
 * Fetch printer status from ESL Manager server
 */
DisplaySmartPrinter::PrinterStatus DisplaySmartPrinter::fetchPrinterStatus(const String& macAddress) {
  PrinterStatus status;
  status.printerName = "Unregistered";
  status.status = "UNKNOWN";
  status.jobId = "--";
  status.orderNumber = "--";
  status.itemNumber = "--";
  status.boxId = "--";
  status.queueCount = 0;
  status.registered = false;
  status.lastUpdate = millis();

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, using defaults");
    status.status = "NO WIFI";
    return status;
  }

  // Build API URL - fetch SmartPrinter data for this device by MAC address
  String url = String(SERVER_URL) + "/api/smartprinter/mac/" + macAddress + "/data";
  Serial.printf("Fetching printer status from: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);  // 10 second timeout

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.printf("Response: %s\n", payload.c_str());

    // Parse JSON response
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      status.registered = true;

      if (doc.containsKey("printer_name")) {
        status.printerName = doc["printer_name"].as<String>();
      }

      if (doc.containsKey("status")) {
        status.status = doc["status"].as<String>();
        status.status.toUpperCase();
      }

      if (doc.containsKey("job_id")) {
        String jobId = doc["job_id"].as<String>();
        status.jobId = (jobId == "0" || jobId == "") ? "--" : jobId;
      }

      if (doc.containsKey("order_number")) {
        String orderNum = doc["order_number"].as<String>();
        status.orderNumber = (orderNum == "0" || orderNum == "") ? "--" : orderNum;
      }

      if (doc.containsKey("item_number")) {
        String itemNum = doc["item_number"].as<String>();
        status.itemNumber = (itemNum == "0" || itemNum == "") ? "--" : itemNum;
      }

      if (doc.containsKey("box_id")) {
        String boxId = doc["box_id"].as<String>();
        status.boxId = (boxId == "0" || boxId == "") ? "--" : boxId;
      }

      if (doc.containsKey("queue_count")) {
        status.queueCount = doc["queue_count"].as<int>();
      }

      Serial.printf("Printer: %s, Status: %s\n", status.printerName.c_str(), status.status.c_str());
    } else {
      Serial.printf("JSON parse error: %s\n", error.c_str());
    }
  } else if (httpCode == 404) {
    Serial.println("Device not registered or no printer assigned (404)");
    status.status = "NOT ASSIGNED";
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    status.status = "ERROR";
  }

  http.end();
  return status;
}

/**
 * Render the SmartPrinter display layout
 * Layout for 400x300 display:
 *
 * +------------------------------------------+
 * | [LOGO]          PRINTER NAME             |
 * |                                          |
 * |           +-----------------------+      |
 * |           |       STATUS          |      |
 * |           +-----------------------+      |
 * |                                          |
 * |  Job ID:        ORDER_NUMBER             |
 * |  Order:         ORDER_NUMBER             |
 * |  Item:          ITEM_NUMBER              |
 * |  Box ID:        BOX_ID                   |
 * |                                          |
 * |                          MAC | 400x300   |
 * +------------------------------------------+
 */
void DisplaySmartPrinter::renderLayout(const PrinterStatus& status) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    int margin = 15;
    int yPos = margin;

    // === TOP SECTION: Logo and Printer Name ===

    // Draw 3DPE logo in top left (64x64)
    drawLogo(margin, yPos);

    // Printer name - large, bold, next to logo
    display->setFont(&FreeSansBold18pt7b);
    int nameX = margin + LOGO_3DPE_WIDTH + 15;
    int nameY = yPos + 40;

    // Truncate name if too long
    String displayName = status.printerName;
    if (displayName.length() > 14) {
      displayName = displayName.substring(0, 13) + "...";
    }
    display->setCursor(nameX, nameY);
    display->print(displayName);

    yPos += LOGO_3DPE_HEIGHT + 20;

    // === STATUS BOX ===
    int statusBoxX = margin;
    int statusBoxY = yPos;
    int statusBoxW = DISPLAY_WIDTH - (2 * margin);
    int statusBoxH = 50;

    // Draw status box with thick border
    display->drawRect(statusBoxX, statusBoxY, statusBoxW, statusBoxH, GxEPD_BLACK);
    display->drawRect(statusBoxX + 1, statusBoxY + 1, statusBoxW - 2, statusBoxH - 2, GxEPD_BLACK);

    // Status text - centered in box
    display->setFont(&FreeSansBold24pt7b);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display->getTextBounds(status.status, 0, 0, &tbx, &tby, &tbw, &tbh);
    int statusX = statusBoxX + (statusBoxW - tbw) / 2;
    int statusY = statusBoxY + (statusBoxH + tbh) / 2;
    display->setCursor(statusX, statusY);
    display->print(status.status);

    yPos += statusBoxH + 25;

    // === DATA FIELDS ===
    int labelX = margin;
    int valueX = margin + 120;
    int lineSpacing = 35;

    display->setFont(&FreeSans12pt7b);

    // Job ID
    display->setCursor(labelX, yPos);
    display->print("Job ID:");
    display->setFont(&FreeSansBold12pt7b);
    display->setCursor(valueX, yPos);
    display->print(status.jobId);
    yPos += lineSpacing;

    // Order Number
    display->setFont(&FreeSans12pt7b);
    display->setCursor(labelX, yPos);
    display->print("Order:");
    display->setFont(&FreeSansBold12pt7b);
    display->setCursor(valueX, yPos);
    display->print(status.orderNumber);
    yPos += lineSpacing;

    // Item Number
    display->setFont(&FreeSans12pt7b);
    display->setCursor(labelX, yPos);
    display->print("Item:");
    display->setFont(&FreeSansBold12pt7b);
    display->setCursor(valueX, yPos);
    display->print(status.itemNumber);
    yPos += lineSpacing;

    // Box ID
    display->setFont(&FreeSans12pt7b);
    display->setCursor(labelX, yPos);
    display->print("Box ID:");
    display->setFont(&FreeSansBold12pt7b);
    display->setCursor(valueX, yPos);
    display->print(status.boxId);

    // === FOOTER ===
    display->setFont(&FreeSans9pt7b);

    // MAC address - bottom left
    display->setCursor(margin, DISPLAY_HEIGHT - 10);
    display->print(formatMacAddress(WiFi.macAddress()));

    // Resolution - bottom right
    String resolution = String(DISPLAY_WIDTH) + "x" + String(DISPLAY_HEIGHT);
    display->getTextBounds(resolution, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(DISPLAY_WIDTH - margin - tbw, DISPLAY_HEIGHT - 10);
    display->print(resolution);

    // Registration indicator border
    if (!status.registered) {
      display->drawRect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, GxEPD_BLACK);
    }

  } while (display->nextPage());
}

/**
 * Draw 3DPE logo
 */
void DisplaySmartPrinter::drawLogo(int x, int y) {
  display->drawBitmap(x, y, logo_3dpe_64x64, LOGO_3DPE_WIDTH, LOGO_3DPE_HEIGHT, GxEPD_BLACK);
}

/**
 * Format MAC address for display
 */
String DisplaySmartPrinter::formatMacAddress(const String& mac) {
  String formatted = mac;
  formatted.toUpperCase();
  return formatted;
}

/**
 * Show error message on display
 */
void DisplaySmartPrinter::showError(const String& message) {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // Draw error border
    display->drawRect(5, 5, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 10, GxEPD_BLACK);
    display->drawRect(6, 6, DISPLAY_WIDTH - 12, DISPLAY_HEIGHT - 12, GxEPD_BLACK);

    // ERROR title
    display->setFont(&FreeSansBold18pt7b);
    display->setCursor(DISPLAY_WIDTH / 2 - 50, 80);
    display->print("ERROR");

    // Error message
    display->setFont(&FreeSans12pt7b);
    display->setCursor(20, 150);
    display->print(message);

  } while (display->nextPage());
}

/**
 * Show "Connecting..." message
 */
void DisplaySmartPrinter::showConnecting() {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    // Draw logo centered
    drawLogo((DISPLAY_WIDTH - LOGO_3DPE_WIDTH) / 2, 60);

    // Connecting text
    display->setFont(&FreeSansBold18pt7b);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    String text = "Connecting...";
    display->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor((DISPLAY_WIDTH - tbw) / 2, 180);
    display->print(text);

    // WiFi SSID if available
    display->setFont(&FreeSans12pt7b);
    if (WiFi.status() == WL_CONNECTED) {
      String ssid = "WiFi: " + WiFi.SSID();
      display->getTextBounds(ssid, 0, 0, &tbx, &tby, &tbw, &tbh);
      display->setCursor((DISPLAY_WIDTH - tbw) / 2, 220);
      display->print(ssid);
    }

  } while (display->nextPage());
}

/**
 * Show "Waiting for data..." message
 */
void DisplaySmartPrinter::showWaiting() {
  display->setFullWindow();
  display->firstPage();

  do {
    display->fillScreen(GxEPD_WHITE);

    display->setFont(&FreeSansBold12pt7b);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    String text = "Waiting for data...";
    display->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor((DISPLAY_WIDTH - tbw) / 2, DISPLAY_HEIGHT / 2);
    display->print(text);

  } while (display->nextPage());
}

/**
 * Get status indicator character
 */
char DisplaySmartPrinter::getStatusIndicator(const String& status) {
  if (status == "READY" || status == "IDLE") return '+';
  if (status == "PRINTING") return '>';
  if (status == "PAUSED") return '|';
  if (status == "ERROR" || status == "OFFLINE") return '!';
  return '?';
}
