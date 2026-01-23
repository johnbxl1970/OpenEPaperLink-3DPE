/**
 * battery.h
 *
 * Battery monitoring for XIAO ESP32C3 with ePaper Driver Board
 * Reads battery voltage via ADC with voltage divider
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

// Pin definitions for ePaper Driver Board battery monitoring
// Using GPIO numbers directly for ESP32C3 compatibility
// Note: GPIO2-5 are used by ePaper display
// Try multiple pins to find the correct one for your board
#define BATTERY_VOLTAGE_PIN 0     // GPIO0 - Try this pin for battery voltage
#define BATTERY_ADC_ENABLE  -1    // Set to -1 to disable ADC enable (not all boards have this)

// Battery voltage thresholds (in millivolts)
// LiPo battery: 4.2V full, 3.0V empty (safe cutoff)
#define BATTERY_FULL_MV     4200
#define BATTERY_EMPTY_MV    3000

// Voltage divider ratio (board has 1:1 divider, so multiply ADC by 2)
#define BATTERY_DIVIDER_RATIO 2

// Number of samples for averaging
#define BATTERY_SAMPLE_COUNT 16

/**
 * Battery status structure
 */
struct BatteryStatus {
  int voltage_mv;       // Battery voltage in millivolts
  int percentage;       // Battery percentage (0-100)
  bool is_charging;     // True if USB power detected (charging)
  bool is_low;          // True if battery is below 20%
  bool is_critical;     // True if battery is below 10%
  bool is_valid;        // True if reading is valid (battery monitoring available)
};

// Valid voltage range for LiPo battery (readings outside this are invalid)
#define BATTERY_VALID_MIN_MV  2500   // Below this is likely noise/invalid
#define BATTERY_VALID_MAX_MV  4500   // Above this is likely invalid

/**
 * BatteryMonitor class
 * Handles battery voltage reading and status calculation
 */
class BatteryMonitor {
public:
  /**
   * Initialize the battery monitor
   * Sets up ADC pins and configuration
   */
  static void init();

  /**
   * Read current battery voltage
   * @return Voltage in millivolts
   */
  static int readVoltage();

  /**
   * Calculate battery percentage from voltage
   * @param voltage_mv Voltage in millivolts
   * @return Percentage (0-100)
   */
  static int calculatePercentage(int voltage_mv);

  /**
   * Get full battery status
   * @return BatteryStatus structure with all battery info
   */
  static BatteryStatus getStatus();

  /**
   * Check if running on USB power (likely charging)
   * @return true if USB voltage detected
   */
  static bool isOnUSBPower();

  /**
   * Print battery status to Serial for debugging
   */
  static void printStatus();

private:
  static bool initialized;
};

#endif // BATTERY_H
