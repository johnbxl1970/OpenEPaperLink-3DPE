/**
 * battery.cpp
 *
 * Battery monitoring implementation for XIAO ESP32C3 with ePaper Driver Board
 */

#include "battery.h"

// Static member initialization
bool BatteryMonitor::initialized = false;

/**
 * Initialize the battery monitor
 */
void BatteryMonitor::init() {
  if (initialized) return;

  // Configure ADC enable pin as output (if used)
  #if BATTERY_ADC_ENABLE >= 0
  pinMode(BATTERY_ADC_ENABLE, OUTPUT);
  digitalWrite(BATTERY_ADC_ENABLE, LOW);  // Start with ADC disabled
  #endif

  // Configure voltage pin as input
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  // Set ADC resolution to 12-bit (0-4095)
  analogReadResolution(12);

  initialized = true;
  Serial.printf("Battery monitor initialized (voltage pin: GPIO%d)\n", BATTERY_VOLTAGE_PIN);
}

/**
 * Read current battery voltage with averaging
 */
int BatteryMonitor::readVoltage() {
  if (!initialized) {
    init();
  }

  // Enable ADC reading (if enable pin is used)
  #if BATTERY_ADC_ENABLE >= 0
  digitalWrite(BATTERY_ADC_ENABLE, HIGH);
  delay(50);  // Allow voltage to stabilize
  #endif

  delay(10);  // Small stabilization delay

  // Take multiple samples and average
  long totalMv = 0;
  for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    // Use analogReadMilliVolts for calibrated reading
    int reading = analogReadMilliVolts(BATTERY_VOLTAGE_PIN);
    totalMv += reading;
    delay(5);  // Small delay between samples
  }

  int avgMv = totalMv / BATTERY_SAMPLE_COUNT;

  // Debug output
  Serial.printf("[Battery] Raw ADC avg: %d mV (GPIO%d)\n", avgMv, BATTERY_VOLTAGE_PIN);

  // Disable ADC to save power (if enable pin is used)
  #if BATTERY_ADC_ENABLE >= 0
  digitalWrite(BATTERY_ADC_ENABLE, LOW);
  #endif

  // Calculate battery voltage with voltage divider ratio
  int batteryMv = avgMv * BATTERY_DIVIDER_RATIO;

  return batteryMv;
}

/**
 * Calculate battery percentage from voltage
 * Uses linear interpolation between empty and full voltages
 */
int BatteryMonitor::calculatePercentage(int voltage_mv) {
  if (voltage_mv >= BATTERY_FULL_MV) {
    return 100;
  }
  if (voltage_mv <= BATTERY_EMPTY_MV) {
    return 0;
  }

  // Linear interpolation
  int range = BATTERY_FULL_MV - BATTERY_EMPTY_MV;
  int level = voltage_mv - BATTERY_EMPTY_MV;
  int percentage = (level * 100) / range;

  return constrain(percentage, 0, 100);
}

/**
 * Get full battery status
 */
BatteryStatus BatteryMonitor::getStatus() {
  BatteryStatus status;

  status.voltage_mv = readVoltage();

  // Check if reading is valid (within expected LiPo voltage range)
  status.is_valid = (status.voltage_mv >= BATTERY_VALID_MIN_MV &&
                     status.voltage_mv <= BATTERY_VALID_MAX_MV);

  if (status.is_valid) {
    status.percentage = calculatePercentage(status.voltage_mv);
    status.is_charging = isOnUSBPower();
    status.is_low = (status.percentage <= 20);
    status.is_critical = (status.percentage <= 10);
  } else {
    // Invalid reading - set default values
    status.percentage = -1;  // -1 indicates N/A
    status.is_charging = false;
    status.is_low = false;
    status.is_critical = false;
  }

  return status;
}

/**
 * Check if running on USB power
 * When USB is connected, voltage will typically read higher or at max
 */
bool BatteryMonitor::isOnUSBPower() {
  int voltage = readVoltage();
  // If voltage is at or above full charge, likely on USB power
  // Also check if voltage is unusually high (USB direct power ~5V)
  return (voltage >= BATTERY_FULL_MV || voltage > 4500);
}

/**
 * Print battery status to Serial
 */
void BatteryMonitor::printStatus() {
  BatteryStatus status = getStatus();

  Serial.println("=== Battery Status ===");
  Serial.printf("Voltage: %d mV\n", status.voltage_mv);
  Serial.printf("Percentage: %d%%\n", status.percentage);
  Serial.printf("Charging: %s\n", status.is_charging ? "Yes" : "No");
  Serial.printf("Low Battery: %s\n", status.is_low ? "Yes" : "No");
  Serial.printf("Critical: %s\n", status.is_critical ? "Yes" : "No");
  Serial.println("=====================");

  // Scan all ADC pins to help find the battery voltage pin
  Serial.println("=== ADC Pin Scan ===");
  int adcPins[] = {0, 1, 2, 3, 4};  // Available ADC1 pins on ESP32C3
  for (int i = 0; i < 5; i++) {
    int pin = adcPins[i];
    int mv = analogReadMilliVolts(pin);
    Serial.printf("GPIO%d: %d mV (x2 = %d mV)\n", pin, mv, mv * 2);
  }
  Serial.println("====================");
}
