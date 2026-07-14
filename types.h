/*
 * AutoCamp Data Types & Structures
 * Centralized type definitions
 */

#ifndef TYPES_H
#define TYPES_H

#pragma pack(1)  // Minimize struct size

// ============ SENSOR DATA STRUCTURES ============

struct SensorData {
  int16_t data24H[24];  // 24-hour history
  int16_t data1H[12];   // 1-hour history (5-min intervals)
};

struct Sensor {
  uint8_t type;           // T_VOLT, T_TEMP, etc.
  uint16_t id;            // Sensor ID from RF module
  uint8_t active;         // 0=inactive, 1=active, 2=initializing
  uint8_t options;        // Bitfield: alarm, raport, etc.
  uint8_t icon;           // Icon index
  int16_t val;            // Current value (x10 for display)
  int16_t min;            // Min threshold
  int16_t max;            // Max threshold
  int16_t alarm_min;      // Alarm threshold (low)
  int16_t alarm_max;      // Alarm threshold (high)
  char name[11];          // "SENSOR1\0"
};

// ============ SETTINGS STRUCTURES ============

struct Settings {
  uint8_t sensors_key;    // Validation key (SENSORS_KEY)
  uint16_t alarm;         // Alarm time (G*60+m)
  uint8_t mp3_volume;     // 0-30
  uint16_t mp3_start;     // Track ID
  uint16_t mp3_click;     // Click sound
  uint16_t mp3_alarm;     // Alarm sound
  uint16_t mp3_1;         // Custom sound 1
  uint16_t mp3_2;         // Custom sound 2
  uint16_t mp3_3;         // Custom sound 3
  uint16_t mp3_4;         // Custom sound 4
  
  char gsm_network[11];   // Network name ("Plus", "Orange", etc)
  char mobile1[11];       // Primary phone number
  char mobile2[11];       // Secondary phone
  char mobile3[11];       // Tertiary phone
  
  DateTime StartTime;     // Trip start time
};

// ============ GSM/MODEM STRUCTURES ============

struct GSMState {
  bool connected;         // Connected to network
  uint8_t cmd;            // Current command
  String result;          // Last result
};

// ============ DISPLAY & TOUCH STRUCTURES ============

struct TouchBox {
  uint16_t x;             // X coordinate
  uint16_t y;             // Y coordinate
  uint8_t w;              // Width
  uint8_t h;              // Height
  uint16_t c;             // Color
  int id;                 // Button ID
};

struct MousePos {
  uint16_t x;
  uint16_t y;
  uint16_t c;
};

// ============ MODULE TYPE STRUCTURE ============

struct ModuleType {
  uint8_t module;         // Module number (first digit)
  uint8_t type;           // Type (second digit)
  uint8_t nr;             // Number (third digit)
};

// ============ RF24 DATA STRUCTURES ============

struct RF24Data {
  int16_t cmd;            // Command (CMD_GET, CMD_SET, etc)
  int16_t id;             // Sensor ID
  int16_t val;            // Value
};

// ============ TIMING STRUCTURES ============

struct Timer {
  uint32_t lastTime;      // Last execution time
  uint32_t interval;      // Interval in milliseconds
  
  bool isReady() const {
    return (millis() - lastTime) >= interval;
  }
  
  void reset() {
    lastTime = millis();
  }
};

// ============ DISPLAY LAYOUT CONFIGURATION ============

struct LayoutConfig {
  const uint16_t buttonW = BUTTON_W;
  const uint16_t buttonH = BUTTON_H;
  const uint16_t buttonTW = BUTTON_TW;
  const uint16_t buttonTH = BUTTON_TH;
  const uint8_t buttonSpacing = 15;
  const uint8_t buttonRound = BUTTON_ROUND;
  
  const uint16_t graphX = GRAPH_X;
  const uint16_t graphY = GRAPH_Y;
  const uint16_t graphW = GRAPH_W;
  const uint16_t graphH = GRAPH_H;
  
  uint16_t getButtonX(uint8_t col) const {
    return 18 + col * (buttonW + buttonSpacing);
  }
  
  uint16_t getButtonY(uint8_t row) const {
    return 40 + row * (buttonH + buttonSpacing);
  }
};

// ============ STATE MACHINE STRUCTURES ============

enum ScreenState {
  SCREEN_MAIN = E_MAIN,
  SCREEN_DEVICE = E_DEVICE,
  SCREEN_SETUP = E_SETUP,
  SCREEN_GRAPH = E_GRAPH,
  SCREEN_SOUND = E_SOUND,
  SCREEN_SENSORS = E_SENSORS,
  SCREEN_SENSOR = E_SENSOR,
  SCREEN_ICONS = E_ICONS,
  SCREEN_MODEM = E_MODEM,
  SCREEN_DATETIME = E_DATETIME
};

enum RFState {
  RF_IDLE,
  RF_WAITING,
  RF_TIMEOUT,
  RF_ERROR
};

enum ModemState {
  MODEM_IDLE,
  MODEM_INIT,
  MODEM_CONNECTING,
  MODEM_CONNECTED,
  MODEM_ERROR
};

// ============ APPLICATION STATE ============

struct AppState {
  volatile ScreenState currentScreen;
  volatile ScreenState previousScreen;
  uint8_t currentPage;
  uint8_t selectedSensor;
  
  bool needsRedraw;
  uint32_t lastRefresh;
  
  bool isInitialized;
  bool isError;
};

// ============ SENSOR READING STATE ============

struct SensorReadState {
  uint8_t sensorIndex;
  uint32_t lastReadTime;
  bool isReading;
  int16_t tempValue;
};

// ============ ENCODER INPUT STATE ============

struct EncoderState {
  volatile int16_t position;
  volatile bool buttonPressed;
  uint32_t lastInteractionTime;
  bool enabled;
};

// ============ GRAPH RENDER STATE ============

struct GraphState {
  uint8_t sensorIndex;
  uint8_t timeRange;        // 1 or 24 (hours)
  uint8_t graphStepX;       // Pixels per data point
  int16_t minValue;
  int16_t maxValue;
  bool needsRedraw;
};

// ============ RF COMMUNICATION STATE ============

struct RFComState {
  RFState state;
  RF24Data sendBuffer;
  RF24Data recvBuffer;
  uint32_t timeout;
  uint8_t retryCount;
  uint8_t maxRetries;
};

#pragma pack()  // Reset packing

// ============ TYPE DEFINITIONS ============

typedef struct Sensor sensor_t;
typedef struct SensorData sensor_data_t;
typedef struct Settings settings_t;
typedef struct GSMState gsm_t;
typedef struct TouchBox box_t;
typedef struct MousePos mouse_t;
typedef struct ModuleType type_module_t;
typedef struct RF24Data s_rf_data;
typedef struct Timer timer_t;
typedef struct LayoutConfig layout_config_t;
typedef struct AppState app_state_t;
typedef struct SensorReadState sensor_read_state_t;
typedef struct EncoderState encoder_state_t;
typedef struct GraphState graph_state_t;
typedef struct RFComState rf_com_state_t;

#endif // TYPES_H
