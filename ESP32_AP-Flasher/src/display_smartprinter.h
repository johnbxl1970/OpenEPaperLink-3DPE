/**
 * display_smartprinter.h
 *
 * SmartPrinter Display Driver for XIAO ESP32C3
 * 4.2" Monochrome ePaper Display (400x300 pixels)
 *
 * Hardware:
 * - Seeed XIAO ESP32C3
 * - 4.2" Monochrome ePaper (400x300 pixels)
 * - SPI interface
 *
 * Place this file in: ESP32_AP-Flasher/src/display_smartprinter.h
 */

#ifndef DISPLAY_SMARTPRINTER_H
#define DISPLAY_SMARTPRINTER_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>

// ePaper display pin definitions for XIAO ESP32C3
// Adjust these if using a different wiring
#define EPD_CS      D7   // Chip Select
#define EPD_DC      D3   // Data/Command
#define EPD_RST     D2   // Reset
#define EPD_BUSY    D1   // Busy signal

// Display dimensions for 4.2" display
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH   400
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT  300
#endif

class DisplaySmartPrinter {
public:
  // Printer status structure
  struct PrinterStatus {
    String printerName;
    String status;
    String jobId;
    String orderNumber;
    String itemNumber;
    String boxId;
    int queueCount;
    bool registered;
    unsigned long lastUpdate;
  };

  /**
   * Initialize the ePaper display
   */
  static void init();

  /**
   * Show the startup/status screen
   */
  static void showStartupScreen();

  /**
   * Update display with printer status from server
   */
  static void updateStatus();

  /**
   * Fetch printer status from ESL Manager server
   * @param macAddress Device MAC address
   * @return PrinterStatus structure with printer details
   */
  static PrinterStatus fetchPrinterStatus(const String& macAddress);

  /**
   * Render the SmartPrinter display layout
   * @param status Printer status information
   */
  static void renderLayout(const PrinterStatus& status);

  /**
   * Draw 3DPE logo
   * @param x X position
   * @param y Y position
   */
  static void drawLogo(int x, int y);

  /**
   * Format MAC address for display
   * @param mac Raw MAC address
   * @return Formatted MAC address string
   */
  static String formatMacAddress(const String& mac);

  /**
   * Show error message on display
   * @param message Error message to display
   */
  static void showError(const String& message);

  /**
   * Show "Connecting..." message
   */
  static void showConnecting();

  /**
   * Show "Waiting for data..." message
   */
  static void showWaiting();

  /**
   * Get status color indicator (for multi-color displays)
   * @param status Status string
   * @return Status indicator character
   */
  static char getStatusIndicator(const String& status);

private:
  // 4.2" GDEY042T81 display (400x300)
  static GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>* display;
};

#endif // DISPLAY_SMARTPRINTER_H
