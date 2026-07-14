/*
 * AutoCamp Sensor Manager
 * Modular sensor management, data logging, and alarm handling
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "config.h"
#include "types.h"
#include "utils.h"

// ============ SENSOR MANAGER CLASS ============

class SensorManager {
private:
  Sensor sensors[MaxSensors];
  SensorData sensorData[MaxSensors];
  
  uint32_t lastReadTime;
  uint8_t lastReadMinute;
  uint8_t lastReadHour;
  
  // Alarm callbacks
  typedef void (*AlarmCallback)(uint8_t sensorIndex, int16_t value);
  AlarmCallback onAlarmTriggered;
  AlarmCallback onAlarmCleared;
  
public:
  
  // ============ INITIALIZATION ============
  
  /**
   * Initialize sensor manager
   */
  SensorManager() : lastReadTime(0), lastReadMinute(255), lastReadHour(255),
                    onAlarmTriggered(nullptr), onAlarmCleared(nullptr) {
    memset(sensors, 0, sizeof(sensors));
    memset(sensorData, 0, sizeof(sensorData));
  }
  
  /**
   * Register alarm triggered callback
   */
  void setAlarmTriggeredCallback(AlarmCallback callback) {
    onAlarmTriggered = callback;
  }
  
  /**
   * Register alarm cleared callback
   */
  void setAlarmClearedCallback(AlarmCallback callback) {
    onAlarmCleared = callback;
  }
  
  /**
   * Load sensors from EEPROM
   */
  bool loadFromEEPROM(uint16_t offset = 200) {
    // Validate offset
    if (offset + sizeof(sensors) > EEPROM.length()) {
      debugPrint("ERROR: Sensor EEPROM offset out of range");
      return false;
    }
    
    EEPROM.get(offset, sensors);
    
    // Validate loaded data
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (!isValidSensor(sensors[i])) {
        debugPrintInt("Invalid sensor data at index", i);
        return false;
      }
    }
    
    return true;
  }
  
  /**
   * Save sensors to EEPROM
   */
  bool saveToEEPROM(uint16_t offset = 200) {
    // Validate offset
    if (offset + sizeof(sensors) > EEPROM.length()) {
      debugPrint("ERROR: Sensor EEPROM offset out of range");
      return false;
    }
    
    EEPROM.put(offset, sensors);
    return true;
  }
  
  // ============ SENSOR ACCESS ============
  
  /**
   * Get sensor by index
   */
  Sensor* getSensor(uint8_t index) {
    if (index >= MaxSensors) return nullptr;
    return &sensors[index];
  }
  
  /**
   * Get sensor data by index
   */
  SensorData* getSensorData(uint8_t index) {
    if (index >= MaxSensors) return nullptr;
    return &sensorData[index];
  }
  
  /**
   * Find sensor by ID
   */
  int findSensorById(uint16_t sensorId) {
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].id == sensorId) {
        return i;
      }
    }
    return -1;
  }
  
  /**
   * Find sensor by name
   */
  int findSensorByName(const char* name) {
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (strcmp(sensors[i].name, name) == 0) {
        return i;
      }
    }
    return -1;
  }
  
  /**
   * Find first active sensor of type
   */
  int findSensorByType(uint8_t sensorType) {
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].type == sensorType && sensors[i].active) {
        return i;
      }
    }
    return -1;
  }
  
  /**
   * Count active sensors
   */
  uint8_t countActiveSensors() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].active) {
        count++;
      }
    }
    return count;
  }
  
  /**
   * Count sensors of specific type
   */
  uint8_t countSensorsByType(uint8_t sensorType) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].type == sensorType) {
        count++;
      }
    }
    return count;
  }
  
  // ============ SENSOR VALUE MANAGEMENT ============
  
  /**
   * Update sensor value with constraints
   */
  bool updateSensorValue(uint8_t index, int16_t value) {
    if (index >= MaxSensors) return false;
    
    // Apply constraints
    value = constrainSensor(value, sensors[index].min, sensors[index].max);
    
    // Update value
    sensors[index].val = value;
    
    // Check for alarm conditions
    checkAlarmCondition(index);
    
    return true;
  }
  
  /**
   * Get sensor display value (already divided by 10)
   */
  int16_t getDisplayValue(uint8_t index) {
    if (index >= MaxSensors) return 0;
    return sensorToDisplay(sensors[index].val);
  }
  
  /**
   * Increment active counter (for RF data reception)
   */
  void incrementActiveCounter(uint8_t index) {
    if (index < MaxSensors && sensors[index].active < 2) {
      sensors[index].active++;
    }
  }
  
  // ============ DATA HISTORY MANAGEMENT ============
  
  /**
   * Add value to 24-hour history
   */
  void addHistoryValue24H(uint8_t index, int16_t value) {
    if (index >= MaxSensors) return;
    
    DateTime now = rtc.now();
    uint8_t hour = now.hour();
    
    if (hour < MaxSensorsData24H) {
      sensorData[index].data24H[hour] = value;
    }
  }
  
  /**
   * Add value to 1-hour history (5-minute intervals)
   */
  void addHistoryValue1H(uint8_t index, int16_t value) {
    if (index >= MaxSensors) return;
    
    DateTime now = rtc.now();
    uint8_t minute = now.minute();
    uint8_t slot = minute / 5;
    
    if (slot < MaxSensorsData1H) {
      sensorData[index].data1H[slot] = value;
    }
  }
  
  /**
   * Update history based on current time (call regularly)
   */
  void updateHistory(uint8_t index) {
    if (index >= MaxSensors) return;
    
    DateTime now = rtc.now();
    uint8_t currentHour = now.hour();
    uint8_t currentMinute = now.minute();
    
    // Update 24-hour history every hour
    if (currentHour != lastReadHour) {
      addHistoryValue24H(index, sensors[index].val);
      lastReadHour = currentHour;
    }
    
    // Update 1-hour history every 5 minutes
    if ((currentMinute / 5) != lastReadMinute) {
      addHistoryValue1H(index, sensors[index].val);
      lastReadMinute = currentMinute / 5;
    }
  }
  
  /**
   * Get average of 24-hour data
   */
  int16_t getAverage24H(uint8_t index) {
    if (index >= MaxSensors) return 0;
    return arrayAverage(sensorData[index].data24H, MaxSensorsData24H);
  }
  
  /**
   * Get average of 1-hour data
   */
  int16_t getAverage1H(uint8_t index) {
    if (index >= MaxSensors) return 0;
    return arrayAverage(sensorData[index].data1H, MaxSensorsData1H);
  }
  
  /**
   * Get minimum of 24-hour data
   */
  int16_t getMin24H(uint8_t index) {
    if (index >= MaxSensors) return 0;
    return arrayMin(sensorData[index].data24H, MaxSensorsData24H);
  }
  
  /**
   * Get maximum of 24-hour data
   */
  int16_t getMax24H(uint8_t index) {
    if (index >= MaxSensors) return 0;
    return arrayMax(sensorData[index].data24H, MaxSensorsData24H);
  }
  
  /**
   * Clear history data
   */
  void clearHistory(uint8_t index) {
    if (index >= MaxSensors) return;
    memset(&sensorData[index], 0, sizeof(SensorData));
  }
  
  /**
   * Clear all history
   */
  void clearAllHistory() {
    memset(sensorData, 0, sizeof(sensorData));
  }
  
  // ============ ALARM MANAGEMENT ============
  
  /**
   * Check alarm condition for sensor
   */
  void checkAlarmCondition(uint8_t index) {
    if (index >= MaxSensors || !sensorHasAlarm(sensors[index])) {
      return;
    }
    
    int16_t value = sensors[index].val;
    int16_t alarmMin = sensors[index].alarm_min * 10;
    int16_t alarmMax = sensors[index].alarm_max * 10;
    
    bool isInAlarm = (value < alarmMin || value > alarmMax);
    
    // Check if state changed
    bool wasInAlarm = (sensors[index].active > 1);  // Using active as alarm flag
    
    if (isInAlarm && !wasInAlarm) {
      // Alarm triggered
      if (onAlarmTriggered != nullptr) {
        onAlarmTriggered(index, value);
      }
      sensors[index].active = 2;  // Mark as in alarm
    } else if (!isInAlarm && wasInAlarm) {
      // Alarm cleared
      if (onAlarmCleared != nullptr) {
        onAlarmCleared(index, value);
      }
      sensors[index].active = 1;  // Mark as normal
    }
  }
  
  /**
   * Check all sensor alarms
   */
  void checkAllAlarms() {
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].active > 0) {
        checkAlarmCondition(i);
      }
    }
  }
  
  /**
   * Get alarm state for sensor
   */
  bool isInAlarm(uint8_t index) {
    if (index >= MaxSensors) return false;
    return sensors[index].active > 1;
  }
  
  /**
   * Clear alarm state
   */
  void clearAlarm(uint8_t index) {
    if (index >= MaxSensors) return;
    if (sensors[index].active > 1) {
      sensors[index].active = 1;
    }
  }
  
  // ============ CONFIGURATION MANAGEMENT ============
  
  /**
   * Initialize sensor with defaults based on ID
   */
  void initializeSensorWithDefaults(uint8_t index) {
    if (index >= MaxSensors) return;
    
    uint8_t type = decodeSensorType(sensors[index].id);
    sensors[index].type = type;
    
    // Set default min/max based on type
    switch (type) {
      case T_VOLT:
        sensors[index].min = 0;
        sensors[index].max = 16;
        break;
      case T_TEMP:
        sensors[index].min = -20;
        sensors[index].max = 50;
        break;
      case T_PROC:
        sensors[index].min = 0;
        sensors[index].max = 100;
        break;
      case T_ANALOG:
        sensors[index].min = 0;
        sensors[index].max = 1023;
        break;
      case T_OUTS:
        sensors[index].min = 0;
        sensors[index].max = 1;
        break;
      default:
        sensors[index].min = 0;
        sensors[index].max = 100;
    }
    
    // Set default alarm thresholds
    sensors[index].alarm_min = sensors[index].min;
    sensors[index].alarm_max = sensors[index].max;
  }
  
  /**
   * Load factory defaults
   */
  void loadFactoryDefaults() {
    // Clear all sensors
    memset(sensors, 0, sizeof(sensors));
    memset(sensorData, 0, sizeof(sensorData));
    
    // Initialize default sensor IDs
    for (uint8_t i = 0; i < MaxSensors; i++) {
      sensors[i].id = 100 + ((i / 3) * 10) + (i % 3);
      sensors[i].active = 0;
      sensors[i].icon = 0;
      sensors[i].options = 0;
      sensors[i].val = 0;
      
      initializeSensorWithDefaults(i);
      
      // Generate default name
      snprintf(sensors[i].name, sizeof(sensors[i].name), "SENS%d", i + 1);
      
      // Populate with random data
      generateRandomData(i);
    }
    
    // Activate some default sensors
    sensors[0].active = 1;  // AKU1
    sensors[1].active = 1;  // AKU2
    sensors[2].active = 1;  // Temp
    sensors[3].active = 1;  // Temp
  }
  
  /**
   * Set sensor configuration
   */
  void configureSensor(uint8_t index, uint16_t sensorId, const char* name,
                      uint8_t icon, uint8_t type) {
    if (index >= MaxSensors) return;
    
    sensors[index].id = sensorId;
    sensors[index].type = type;
    sensors[index].icon = icon;
    
    safeStrcpy(sensors[index].name, sizeof(sensors[index].name), name);
    initializeSensorWithDefaults(index);
  }
  
  /**
   * Enable sensor
   */
  void enableSensor(uint8_t index) {
    if (index < MaxSensors) {
      sensors[index].active = 1;
    }
  }
  
  /**
   * Disable sensor
   */
  void disableSensor(uint8_t index) {
    if (index < MaxSensors) {
      sensors[index].active = 0;
    }
  }
  
  /**
   * Toggle sensor enabled state
   */
  void toggleSensor(uint8_t index) {
    if (index < MaxSensors) {
      sensors[index].active = sensors[index].active ? 0 : 1;
    }
  }
  
  /**
   * Enable alarm for sensor
   */
  void enableAlarm(uint8_t index) {
    if (index < MaxSensors) {
      setSensorOption(sensors[index], O_ALARM, true);
    }
  }
  
  /**
   * Disable alarm for sensor
   */
  void disableAlarm(uint8_t index) {
    if (index < MaxSensors) {
      setSensorOption(sensors[index], O_ALARM, false);
    }
  }
  
  /**
   * Enable reporting for sensor
   */
  void enableReporting(uint8_t index) {
    if (index < MaxSensors) {
      setSensorOption(sensors[index], O_RAPORT, true);
    }
  }
  
  /**
   * Disable reporting for sensor
   */
  void disableReporting(uint8_t index) {
    if (index < MaxSensors) {
      setSensorOption(sensors[index], O_RAPORT, false);
    }
  }
  
  // ============ VALIDATION ============
  
  /**
   * Validate sensor data
   */
  bool isValidSensor(const Sensor& sensor) const {
    // Check for obviously invalid data
    if (sensor.id == 0) return false;
    if (sensor.min > sensor.max) return false;
    if (sensor.type > T_PWM) return false;
    
    return true;
  }
  
  /**
   * Validate sensor value for type
   */
  bool isValidValue(uint8_t index, int16_t value) {
    if (index >= MaxSensors) return false;
    
    int16_t minAllowed = sensors[index].min * 10;
    int16_t maxAllowed = sensors[index].max * 10;
    
    return isValidSensorValue(value, minAllowed, maxAllowed);
  }
  
  // ============ DEBUG & UTILITY ============
  
  /**
   * Generate random test data
   */
  void generateRandomData(uint8_t index) {
    if (index >= MaxSensors) return;
    
    int16_t minVal = sensors[index].min * 10;
    int16_t maxVal = sensors[index].max * 10;
    int16_t range = maxVal - minVal;
    int16_t midPoint = minVal + (range / 2);
    
    // Generate data for 24 hours
    for (uint8_t h = 0; h < MaxSensorsData24H; h++) {
      int16_t noise = random(-range / 4, range / 4);
      sensorData[index].data24H[h] = constrain(midPoint + noise, minVal, maxVal);
    }
    
    // Generate data for 1 hour
    for (uint8_t m = 0; m < MaxSensorsData1H; m++) {
      int16_t noise = random(-range / 6, range / 6);
      sensorData[index].data1H[m] = constrain(midPoint + noise, minVal, maxVal);
    }
  }
  
  /**
   * Print sensor configuration (debug)
   */
  void printSensorConfig(uint8_t index) {
    if (index >= MaxSensors) return;
    
    Serial.print("Sensor ");
    Serial.print(index);
    Serial.print(": ID=");
    Serial.print(sensors[index].id);
    Serial.print(" Name=");
    Serial.print(sensors[index].name);
    Serial.print(" Type=");
    Serial.print(sensors[index].type);
    Serial.print(" Active=");
    Serial.print(sensors[index].active);
    Serial.print(" Min=");
    Serial.print(sensors[index].min);
    Serial.print(" Max=");
    Serial.println(sensors[index].max);
  }
  
  /**
   * Print all sensor configurations
   */
  void printAllConfigs() {
    for (uint8_t i = 0; i < MaxSensors; i++) {
      if (sensors[i].id != 0) {
        printSensorConfig(i);
      }
    }
  }
};

#endif // SENSOR_MANAGER_H
