/**
 * display_3dpe.h
 *
 * 3DPE Custom Startup Screen for XIAO ESP32C3
 * Displays device information and 3DPE branding on 2.9" ePaper display
 *
 * Hardware:
 * - Seeed XIAO ESP32C3
 * - XIAO eInk Expansion Board
 * - 2.9" Monochrome ePaper (296x128 pixels)
 *
 * Place this file in: ESP32_AP-Flasher/src/display_3dpe.h
 */

#ifndef DISPLAY_3DPE_H
#define DISPLAY_3DPE_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>

// ePaper display pin definitions for XIAO eInk Expansion Board
#define EPD_CS      D7  // Chip Select
#define EPD_DC      D3  // Data/Command
#define EPD_RST     D2  // Reset
#define EPD_BUSY    D1  // Busy signal

// Display dimensions
#define DISPLAY_WIDTH   296
#define DISPLAY_HEIGHT  128

class Display3DPE {
public:
  // Device info structure
  struct DeviceInfo {
    String name;
    String deviceType;
    String commType;
    bool registered;
  };

  /**
   * Initialize the ePaper display
   */
  static void init();

  /**
   * Show the startup screen with device information
   * Called on boot or firmware update
   */
  static void showStartupScreen();

  /**
   * Fetch device configuration from ESL Manager server
   * @param macAddress Device MAC address
   * @return DeviceInfo structure with device details
   */
  static DeviceInfo fetchDeviceInfo(const String& macAddress);

  /**
   * Render the complete startup screen layout
   * @param info Device information
   */
  static void renderStartupLayout(const DeviceInfo& info);

  /**
   * Draw 3DPE logo in top right corner
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
   * Clear display and show "Connecting..." message
   */
  static void showConnecting();

private:
  static GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::HEIGHT>* display;
};

#endif // DISPLAY_3DPE_H
