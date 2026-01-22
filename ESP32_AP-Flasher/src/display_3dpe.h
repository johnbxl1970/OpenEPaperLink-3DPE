/**
 * display_3dpe.h
 *
 * 3DPE Custom Startup Screen for XIAO ESP32-C3
 * Displays device information and 3DPE branding on 2.9" ePaper display
 *
 * Hardware:
 * - Seeed XIAO ESP32-C3
 * - Seeed ePaper Driver Board for XIAO V2
 * - 2.9" Monochrome ePaper (296x128 pixels, SSD1680 controller)
 *
 * See README_3DPE.md for complete hardware documentation and pin mappings.
 */

#ifndef DISPLAY_3DPE_H
#define DISPLAY_3DPE_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <WiFi.h>

// ePaper display pin definitions for XIAO ePaper Driver Board V2
// Reference: https://wiki.seeedstudio.com/xiao_eink_expansion_board_v2/
#define EPD_RST     D0  // Reset (GPIO 2)
#define EPD_CS      D1  // Chip Select (GPIO 3)
#define EPD_BUSY    D2  // Busy signal (GPIO 4)
#define EPD_DC      D3  // Data/Command (GPIO 5)

// Display dimensions
#define DISPLAY_WIDTH   296
#define DISPLAY_HEIGHT  128

// 2.9" display driver - try different variants if display doesn't work
// Options: GxEPD2_290_T5, GxEPD2_290_T5D, GxEPD2_290_BS, GxEPD2_290, GxEPD2_290_T94
#define DISPLAY_CLASS GxEPD2_290_BS

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
  static GxEPD2_BW<DISPLAY_CLASS, DISPLAY_CLASS::HEIGHT>* display;
};

#endif // DISPLAY_3DPE_H
