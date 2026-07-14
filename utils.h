/*
 * AutoCamp Utility Functions & Helpers
 * Common utility functions to reduce code duplication
 */

#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include "types.h"

// ============ MEMORY & STRING UTILITIES ============

/**
 * Safe string concatenation with buffer overflow protection
 */
bool safeStrcat(char* dest, size_t destSize, const char* src) {
  size_t destLen = strlen(dest);
  size_t srcLen = strlen(src);
  
  if (destLen + srcLen >= destSize) {
    return false;  // Would overflow
  }
  
  strcat(dest, src);
  return true;
}

/**
 * Safe string copy with bounds checking
 */
bool safeStrcpy(char* dest, size_t destSize, const char* src) {
  if (strlen(src) >= destSize) {
    return false;  // Would overflow
  }
  
  strcpy(dest, src);
  return true;
}

/**
 * Convert integer to string with bounds checking
 */
void intToStr(int value, char* buffer, size_t bufSize, uint8_t width = 3, bool padZero = false) {
  if (bufSize < 12) return;
  
  char format[8];
  if (padZero) {
    snprintf(format, sizeof(format), "%%0%dd", width);
  } else {
    snprintf(format, sizeof(format), "%%%dd", width);
  }
  
  snprintf(buffer, bufSize, format, value);
}

/**
 * Convert float to string with decimal places
 */
void floatToStr(float value, char* buffer, size_t bufSize, uint8_t decimals = 1) {
  if (bufSize < 12) return;
  dtostrf(value, 3, decimals, buffer);
}

/**
 * Safely clear EEPROM memory
 */
void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);
  }
}

// ============ TIMING UTILITIES ============

/**
 * Non-blocking timer check
 */
bool timerElapsed(uint32_t& lastTime, uint32_t interval) {
  if (millis() - lastTime >= interval) {
    lastTime = millis();
    return true;
  }
  return false;
}

/**
 * Calculate time difference in milliseconds
 */
uint32_t timeDiff(uint32_t startTime) {
  return millis() - startTime;
}

/**
 * Check if timeout has occurred
 */
bool isTimeout(uint32_t startTime, uint32_t timeoutMs) {
  return (millis() - startTime) > timeoutMs;
}

// ============ SENSOR VALUE UTILITIES ============

/**
 * Convert raw sensor value to display value (divide by 10)
 */
int16_t sensorToDisplay(int16_t rawValue) {
  return rawValue / 10;
}

/**
 * Convert display value to sensor value (multiply by 10)
 */
int16_t displayToSensor(int16_t displayValue) {
  return displayValue * 10;
}

/**
 * Constrain sensor value within min/max bounds
 */
int16_t constrainSensor(int16_t value, int16_t minVal, int16_t maxVal) {
  if (value < minVal * 10) return minVal * 10;
  if (value > maxVal * 10) return maxVal * 10;
  return value;
}

/**
 * Map sensor value to percentage (0-100)
 */
uint8_t sensorToPercent(int16_t value, int16_t minVal, int16_t maxVal) {
  if (maxVal <= minVal) return 0;
  
  int16_t range = maxVal - minVal;
  int16_t offset = value - (minVal * 10);
  
  uint16_t percent = (offset * 100) / (range * 10);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  
  return (uint8_t)percent;
}

/**
 * Calculate graph Y coordinate from value
 */
int16_t graphPointY(int16_t value, int16_t minVal, int16_t maxVal) {
  if (minVal > maxVal) maxVal = minVal;
  
  int16_t y = map(value, minVal * 10, maxVal * 10, 0, GRAPH_H);
  if (y > GRAPH_H) y = GRAPH_H;
  if (y < 0) y = 0;
  
  return GRAPH_Y - y;
}

// ============ DISPLAY UTILITIES ============

/**
 * Format temperature for display with unit
 */
void formatTemp(int16_t tempValue, char* buffer, size_t bufSize) {
  float temp = sensorToDisplay(tempValue);
  snprintf(buffer, bufSize, "%.1f°C", temp);
}

/**
 * Format voltage for display with unit
 */
void formatVoltage(int16_t voltValue, char* buffer, size_t bufSize) {
  float volt = sensorToDisplay(voltValue);
  snprintf(buffer, bufSize, "%.1fV", volt);
}

/**
 * Format percentage for display with unit
 */
void formatPercent(int16_t value, char* buffer, size_t bufSize) {
  snprintf(buffer, bufSize, "%d%%", value);
}

/**
 * Get color based on alarm state
 */
uint16_t getAlarmColor(int16_t value, int16_t alarmMin, int16_t alarmMax) {
  if (value < alarmMin * 10 || value > alarmMax * 10) {
    return RED;
  }
  return GREEN;
}

// ============ TOUCH UTILITIES ============

/**
 * Check if touch point is within box bounds
 */
bool touchInBox(uint16_t touchX, uint16_t touchY, const TouchBox& box) {
  return (touchX >= box.x && touchX <= (box.x + box.w) &&
          touchY >= box.y && touchY <= (box.y + box.h));
}

/**
 * Find touch ID from coordinates
 */
int findTouchId(uint16_t touchX, uint16_t touchY, const TouchBox* boxes, uint8_t boxCount) {
  for (uint8_t i = 0; i < boxCount; i++) {
    if (touchInBox(touchX, touchY, boxes[i])) {
      return boxes[i].id;
    }
  }
  return K_NONE;
}

// ============ SENSOR MANAGEMENT UTILITIES ============

/**
 * Find sensor by ID
 */
int findSensorById(uint16_t sensorId, const Sensor* sensors, uint8_t sensorCount) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    if (sensors[i].id == sensorId) {
      return i;
    }
  }
  return -1;
}

/**
 * Find sensor by type
 */
int findSensorByType(uint8_t sensorType, const Sensor* sensors, uint8_t sensorCount) {
  for (uint8_t i = 0; i < sensorCount; i++) {
    if (sensors[i].type == sensorType && sensors[i].active) {
      return i;
    }
  }
  return -1;
}

/**
 * Count active sensors
 */
uint8_t countActiveSensors(const Sensor* sensors, uint8_t sensorCount) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < sensorCount; i++) {
    if (sensors[i].active) {
      count++;
    }
  }
  return count;
}

/**
 * Check if sensor has alarm enabled
 */
bool sensorHasAlarm(const Sensor& sensor) {
  return (sensor.options & _BV(O_ALARM)) != 0;
}

/**
 * Check if sensor should send report
 */
bool sensorShouldReport(const Sensor& sensor) {
  return (sensor.options & _BV(O_RAPORT)) != 0;
}

/**
 * Enable/disable sensor option
 */
void setSensorOption(Sensor& sensor, uint8_t option, bool enable) {
  if (enable) {
    sensor.options |= _BV(option);
  } else {
    sensor.options &= ~_BV(option);
  }
}

// ============ DECODER UTILITIES ============

/**
 * Decode sensor type from module ID
 */
uint8_t decodeSensorType(uint16_t moduleId) {
  uint8_t t = moduleId % 100;
  return t / 10;
}

/**
 * Decode sensor number from module ID
 */
uint8_t decodeSensorNumber(uint16_t moduleId) {
  return moduleId % 10;
}

/**
 * Decode module number from module ID
 */
uint8_t decodeModuleNumber(uint16_t moduleId) {
  return moduleId / 100;
}

// ============ CHECKSUM & VALIDATION ============

/**
 * Calculate simple checksum (XOR)
 */
uint8_t calculateChecksum(const uint8_t* data, size_t length) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

/**
 * Verify checksum
 */
bool verifyChecksum(const uint8_t* data, size_t length, uint8_t expectedChecksum) {
  return calculateChecksum(data, length) == expectedChecksum;
}

/**
 * Validate sensor data range
 */
bool isValidSensorValue(int16_t value, int16_t minAllowed, int16_t maxAllowed) {
  return (value >= minAllowed && value <= maxAllowed);
}

// ============ SERIAL COMMUNICATION UTILITIES ============

/**
 * Wait for serial data with timeout
 */
bool waitForSerial(uint16_t timeoutMs) {
  uint32_t startTime = millis();
  while (!Serial.available() && !isTimeout(startTime, timeoutMs)) {
    delay(10);
  }
  return Serial.available();
}

/**
 * Read string from serial with timeout
 */
bool readSerialString(char* buffer, size_t bufSize, uint16_t timeoutMs = 1000) {
  uint32_t startTime = millis();
  uint8_t index = 0;
  
  while (!isTimeout(startTime, timeoutMs) && index < bufSize - 1) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        buffer[index] = '\0';
        return true;
      }
      if (c >= 32) {  // Printable character
        buffer[index++] = c;
      }
    }
    delay(1);
  }
  
  return false;
}

/**
 * Send command and wait for response
 */
bool sendCommandWaitResponse(const char* cmd, char* responseBuffer, size_t bufSize, 
                            uint16_t timeoutMs = 500) {
  Serial1.print(cmd);
  Serial1.print("\r");
  
  return readSerialString(responseBuffer, bufSize, timeoutMs);
}

// ============ DEBUG UTILITIES ============

/**
 * Print debug message with timestamp
 */
void debugPrint(const char* msg) {
#ifdef DEBUG
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] ");
  Serial.println(msg);
#endif
}

/**
 * Print debug integer value
 */
void debugPrintInt(const char* label, int16_t value) {
#ifdef DEBUG
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
#endif
}

/**
 * Print free RAM (for debugging memory issues)
 */
uint16_t freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

/**
 * Print memory status
 */
void printMemoryStatus() {
#ifdef DEBUG
  Serial.print("Free RAM: ");
  Serial.print(freeRam());
  Serial.println(" bytes");
#endif
}

// ============ MATH UTILITIES ============

/**
 * Calculate average of array
 */
int16_t arrayAverage(const int16_t* array, uint8_t length) {
  if (length == 0) return 0;
  
  int32_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += array[i];
  }
  
  return (int16_t)(sum / length);
}

/**
 * Find minimum value in array
 */
int16_t arrayMin(const int16_t* array, uint8_t length) {
  if (length == 0) return 0;
  
  int16_t minVal = array[0];
  for (uint8_t i = 1; i < length; i++) {
    if (array[i] < minVal) {
      minVal = array[i];
    }
  }
  
  return minVal;
}

/**
 * Find maximum value in array
 */
int16_t arrayMax(const int16_t* array, uint8_t length) {
  if (length == 0) return 0;
  
  int16_t maxVal = array[0];
  for (uint8_t i = 1; i < length; i++) {
    if (array[i] > maxVal) {
      maxVal = array[i];
    }
  }
  
  return maxVal;
}

/**
 * Calculate standard deviation
 */
float arrayStdDev(const int16_t* array, uint8_t length) {
  if (length < 2) return 0;
  
  int16_t avg = arrayAverage(array, length);
  float sumSqDiff = 0;
  
  for (uint8_t i = 0; i < length; i++) {
    float diff = array[i] - avg;
    sumSqDiff += diff * diff;
  }
  
  return sqrt(sumSqDiff / length);
}

#endif // UTILS_H
