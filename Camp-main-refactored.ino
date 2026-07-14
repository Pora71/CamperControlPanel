/*
 * AutoCamp Control Panel - Main Sketch (Refactored)
 * Modular architecture with separation of concerns
 * 
 * Hardware: Arduino Mega 2560
 * Display: Adafruit 3.5" TFT LCD 320x480
 * Wireless: nRF24L01+ @ 433MHz
 * RTC: DS3231
 * Modem: SIM800L
 * MP3: CATALEX YX6300
 */

// ============ INCLUDES ============
#include <Wire.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <Encoder.h>
#include <RTClib.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <TouchScreen.h>

// Custom headers
#include "config.h"
#include "types.h"
#include "utils.h"
#include "state_machine.h"
#include "sensor_manager.h"
#include "colors.h"
#include "icons-logo.h"
#include "icons-40x32.h"

// ============ HARDWARE OBJECTS ============

// Display
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts(XP, YP, XM, YM, 300);

// Real Time Clock
RTC_DS3231 rtc;

// RF24 wireless module
RF24 radio(CE_PIN, CSN_PIN);

// Encoder
Encoder myEncoder(ENCODER_1, ENCODER_2);

// ============ APPLICATION STATE ============

// Managers
StateMachine* stateMachine = nullptr;
SensorManager sensorManager;

// Global variables (minimize these!)
uint32_t lastSensorRefresh = 0;
uint32_t lastDisplayUpdate = 0;
uint8_t currentPage = 0;

// Touch and input
char inputString[21] = "";
TouchBox touchBoxes[TOUCH_MAX];
uint8_t touchBoxCount = 0;

// Module status
int MODULES[10] = {0};
bool firstStart = true;

// GSM/Modem
String GSMResult[10];
uint8_t GSMResultCount = 0;
gsm_t gsm = {false, 0, ""};

// RF24 data
s_rf_data dataSend = {0, 0, 0};
s_rf_data dataReceived = {0, 0, 0};

// ============ SETUP ============

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  Serial1.begin(MODEM_BAUD);
  
  while (!Serial) {
    delay(10);
  }
  
  Serial.println(F("\n\n===== AutoCamp v0.2 Starting ====="));
  
  // Initialize I/O pins
  pinMode(ENCODER_push, INPUT_PULLUP);
  pinMode(BOOT_MODEM, OUTPUT);
  digitalWrite(BOOT_MODEM, 0);
  
  // Initialize random seed
  randomSeed(analogRead(15));
  
  // Initialize display
  displayInit();
  
  // Initialize RTC
  rtcInit();
  
  // Initialize RF24
  rfInit();
  
  // Initialize sensors
  sensorInit();
  
  // Create state machine
  stateMachine = createStateMachine();
  if (stateMachine == nullptr) {
    Serial.println(F("ERROR: Failed to create state machine!"));
    while (1) {
      delay(1000);
    }
  }
  
  // Boot modem
  digitalWrite(BOOT_MODEM, 1);
  delay(500);
  
  // Connect GSM
  gsmConnect();
  
  // Show welcome screen
  drawLogo();
  delay(2000);
  
  // Draw main screen
  stateMachine->transitionTo(SCREEN_MAIN);
  drawScreen();
  
  Serial.println(F("Setup complete. System ready."));
  Serial.print(F("Free RAM: "));
  Serial.println(freeRam());
  
  firstStart = true;
}

// ============ MAIN LOOP ============

void loop() {
  // Non-blocking display updates
  if (timerElapsed(lastDisplayUpdate, 100)) {
    handleTouchInput();
    updateDisplay();
  }
  
  // Non-blocking sensor updates
  if (timerElapsed(lastSensorRefresh, REFRESH_SENSORS)) {
    readAllSensors();
    sensorManager.checkAllAlarms();
    stateMachine->setNeedsRedraw(true);
  }
  
  // Process queued events
  stateMachine->processEvents();
  
  // Handle state transitions
  if (stateMachine->isStateTimeout()) {
    stateMachine->transitionTo(SCREEN_MAIN);
  }
  
  // Check for first start report
  if (firstStart) {
    sendWelcomeReport();
    firstStart = false;
  }
}

// ============ INITIALIZATION FUNCTIONS ============

/**
 * Initialize display
 */
void displayInit() {
  Serial.println(F("Initializing display..."));
  tft.reset();
  tft.begin(0x8357);  // HX8357D LCD driver
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  Serial.println(F("Display initialized"));
}

/**
 * Initialize RTC
 */
void rtcInit() {
  Serial.println(F("Initializing RTC..."));
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println(F("ERROR: RTC not found!"));
    return;
  }
  
  if (!rtc.isrunning()) {
    Serial.println(F("RTC is not running. Setting time to compile time..."));
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  
  Serial.println(F("RTC initialized"));
}

/**
 * Initialize RF24 module
 */
void rfInit() {
  Serial.println(F("Initializing RF24..."));
  
  if (!radio.begin()) {
    Serial.println(F("ERROR: RF24 initialization failed!"));
    return;
  }
  
  radio.setChannel(CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.enableDynamicPayloads();
  radio.setRetries(10, 15);
  radio.setCRCLength(RF24_CRC_16);
  
  radio.openWritingPipe(0x3130323354LL);  // "1Node" in hex
  radio.openReadingPipe(1, 0x3230323354LL);  // "2Node" in hex
  
  Serial.println(F("RF24 initialized"));
}

/**
 * Initialize sensors from EEPROM
 */
void sensorInit() {
  Serial.println(F("Initializing sensors..."));
  
  // Load from EEPROM
  if (!sensorManager.loadFromEEPROM(200)) {
    Serial.println(F("Loading factory defaults..."));
    sensorManager.loadFactoryDefaults();
  }
  
  // Set alarm callbacks
  sensorManager.setAlarmTriggeredCallback(onSensorAlarmTriggered);
  sensorManager.setAlarmClearedCallback(onSensorAlarmCleared);
  
  // Initialize all histories
  sensorManager.clearAllHistory();
  
  // Print configuration
  #ifdef DEBUG
    sensorManager.printAllConfigs();
  #endif
  
  Serial.println(F("Sensors initialized"));
}

// ============ MAIN SCREEN DRAWING ============

/**
 * Draw main application screen
 */
void drawScreen() {
  if (!stateMachine->needsRedraw()) {
    return;
  }
  
  switch (stateMachine->getCurrentState()) {
    case SCREEN_MAIN:
      drawMainScreen();
      break;
    case SCREEN_SETUP:
      drawSetupScreen();
      break;
    case SCREEN_SENSORS:
      drawSensorsScreen();
      break;
    case SCREEN_GRAPH:
      drawGraphScreen();
      break;
    case SCREEN_MODEM:
      drawModemScreen();
      break;
    default:
      break;
  }
  
  stateMachine->markRedrawn();
}

/**
 * Draw main monitoring screen
 */
void drawMainScreen() {
  tft.fillScreen(BLACK);
  touchBoxCount = 0;
  
  // Draw header with status icons
  drawHeader();
  
  // Draw sensor buttons
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t sensorIndex = (currentPage * 8) + i;
    if (sensorIndex < MaxSensors) {
      drawSensorButton(i, sensorIndex);
    }
  }
  
  // Draw navigation buttons
  drawNavigationButtons();
}

/**
 * Draw header with status indicators
 */
void drawHeader() {
  DateTime now = rtc.now();
  
  // RF status icon 1
  tft.drawBitmap(0, 0, icon_ant1, 24, 32, MODULES[0] > 0 ? GREEN : RED);
  
  // RF status icon 2
  tft.drawBitmap(24, 0, icon_ant2, 24, 32, MODULES[1] > 0 ? GREEN : RED);
  
  // GSM status icon
  tft.drawBitmap(48, 0, icon_antg, 24, 32, gsm.connected ? GREEN : RED);
  
  // Time and date
  char timeStr[20];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", now.hour(), now.minute());
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(100, 5);
  tft.print(timeStr);
  
  // Version info
  tft.setTextSize(1);
  tft.setCursor(350, 5);
  tft.print(SYSTEM_NAME);
}

/**
 * Draw individual sensor button
 */
void drawSensorButton(uint8_t buttonNum, uint8_t sensorIndex) {
  Sensor* sensor = sensorManager.getSensor(sensorIndex);
  if (sensor == nullptr || sensor->id == 0) {
    return;
  }
  
  // Calculate position
  uint8_t row = buttonNum / 4;
  uint8_t col = buttonNum % 4;
  uint16_t x = 18 + col * (BUTTON_W + 15);
  uint16_t y = 40 + row * (BUTTON_H + 15);
  
  // Draw button background
  uint16_t bgColor = sensor->active ? BLUE : BLUEd;
  tft.fillRoundRect(x, y, BUTTON_W, BUTTON_H, BUTTON_ROUND, bgColor);
  tft.drawRoundRect(x, y, BUTTON_W, BUTTON_H, BUTTON_ROUND, YELLOW);
  
  // Draw sensor icon
  if (sensor->icon < ICONS40_count) {
    uint16_t iconColor = sensor->active ? YELLOW : GRAY;
    tft.drawBitmap(x + 14, y + 7, (const uint8_t*)ICONS40[sensor->icon], 40, 32, iconColor);
  }
  
  // Draw value
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(x + 5, y + 50);
  
  char valueStr[10];
  int16_t displayVal = sensorManager.getDisplayValue(sensorIndex);
  snprintf(valueStr, sizeof(valueStr), "%d", displayVal);
  tft.print(valueStr);
  
  // Add to touch boxes
  addTouchBox(x, y, BUTTON_W, BUTTON_H, sensorIndex + 1);
}

/**
 * Draw navigation and control buttons
 */
void drawNavigationButtons() {
  // Page button
  drawControlButton(18 + 3 * (BUTTON_W + 15), 40 + 2 * (BUTTON_H + 15), 
                    BUTTON_W, BUTTON_H, K_PAGE, F("PAGE"));
  
  // Setup button
  drawControlButton(18 + 4 * (BUTTON_W + 15), 40 + 2 * (BUTTON_H + 15),
                    BUTTON_W, BUTTON_H, K_SETUP, F("SETUP"));
  
  // Alarms button
  drawControlButton(18 + 3 * (BUTTON_W + 15), 40 + 3 * (BUTTON_H + 15),
                    BUTTON_W, BUTTON_H, K_ALARMS, F("ALARM"));
}

/**
 * Draw control button
 */
void drawControlButton(uint16_t x, uint16_t y, uint8_t w, uint8_t h, 
                       int buttonId, const __FlashStringHelper* text) {
  tft.fillRoundRect(x, y, w, h, BUTTON_ROUND, RED);
  tft.drawRoundRect(x, y, w, h, BUTTON_ROUND, YELLOW);
  
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(x + 10, y + 20);
  tft.print(text);
  
  addTouchBox(x, y, w, h, buttonId);
}

/**
 * Draw setup screen
 */
void drawSetupScreen() {
  tft.fillScreen(BLACK);
  touchBoxCount = 0;
  
  drawTitle(F("SETUP"));
  
  drawControlButton(50, 60, 150, 50, K_SENSORS, F("SENSORS"));
  drawControlButton(250, 60, 150, 50, K_SOUND, F("SOUND"));
  
  drawControlButton(50, 150, 150, 50, K_DATETIME, F("DATE/TIME"));
  drawControlButton(250, 150, 150, 50, K_MODEM, F("MODEM"));
  
  drawControlButton(150, 250, 150, 50, K_CANCEL, F("BACK"));
}

/**
 * Draw screen title
 */
void drawTitle(const __FlashStringHelper* title) {
  tft.fillRect(0, 0, LCD_W, 40, BLUE);
  tft.drawFastHLine(0, 0, LCD_W, YELLOW);
  tft.drawFastHLine(0, 40, LCD_W, YELLOW);
  
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(20, 5);
  tft.print(title);
}

/**
 * Draw sensors configuration screen
 */
void drawSensorsScreen() {
  tft.fillScreen(BLACK);
  touchBoxCount = 0;
  
  drawTitle(F("SENSORS"));
  
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(20, 60);
  tft.print(F("Active Sensors: "));
  tft.println(sensorManager.countActiveSensors());
  
  tft.setCursor(20, 100);
  uint8_t y = 100;
  for (uint8_t i = 0; i < MaxSensors && i < 6; i++) {
    Sensor* s = sensorManager.getSensor(i);
    if (s && s->id > 0) {
      tft.setCursor(20, y);
      tft.print(i);
      tft.print(": ");
      tft.print(s->name);
      tft.print(" (");
      tft.print(s->active ? "ON" : "OFF");
      tft.println(")");
      y += 30;
    }
  }
}

/**
 * Draw graph screen
 */
void drawGraphScreen() {
  tft.fillScreen(BLACK);
  touchBoxCount = 0;
  
  uint8_t sensorIndex = stateMachine->getSelectedSensor();
  Sensor* sensor = sensorManager.getSensor(sensorIndex);
  
  if (sensor == nullptr) {
    return;
  }
  
  drawTitle(sensor->name);
  
  // Draw simple bar graph
  int16_t val = sensorManager.getDisplayValue(sensorIndex);
  uint8_t percent = sensorToPercent(val, sensor->min, sensor->max);
  
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(50, 100);
  tft.print(F("Value: "));
  tft.println(val);
  
  // Draw bar
  uint16_t barWidth = percent * 4;  // Scale to display width
  tft.fillRect(50, 200, barWidth, 50, GREEN);
  tft.drawRect(50, 200, 400, 50, YELLOW);
  
  tft.setCursor(50, 260);
  tft.print(percent);
  tft.println(F("%"));
}

/**
 * Draw modem screen
 */
void drawModemScreen() {
  tft.fillScreen(BLACK);
  touchBoxCount = 0;
  
  drawTitle(F("MODEM"));
  
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  
  tft.setCursor(50, 60);
  tft.print(F("Status: "));
  tft.println(gsm.connected ? "CONNECTED" : "DISCONNECTED");
  
  drawControlButton(50, 120, 150, 50, K_TEST, F("TEST"));
  drawControlButton(250, 120, 150, 50, K_RESET, F("RESET"));
  
  drawControlButton(150, 220, 150, 50, K_CANCEL, F("BACK"));
}

/**
 * Draw logo screen
 */
void drawLogo() {
  tft.fillScreen(BLACK);
  
  tft.setTextSize(4);
  tft.setTextColor(WHITE);
  tft.setCursor(100, 80);
  tft.print(F("AutoCamp"));
  
  tft.setTextSize(2);
  tft.setCursor(120, 150);
  tft.print(F("v0.2"));
  
  tft.setTextSize(1);
  tft.setCursor(100, 250);
  tft.print(F("Initializing..."));
}

// ============ DISPLAY UPDATE ============

/**
 * Update display
 */
void updateDisplay() {
  drawScreen();
}

// ============ INPUT HANDLING ============

/**
 * Handle touch input
 */
void handleTouchInput() {
  TSPoint touch = readTouch(0);
  
  if (touch.z > MINPRESSURE) {
    int buttonId = findTouchIdFromCoords(touch.x, touch.y);
    
    if (buttonId != K_NONE) {
      Serial.print(F("Button pressed: "));
      Serial.println(buttonId);
      
      stateMachine->handleButtonPress(buttonId);
    }
  }
}

/**
 * Read touch point
 */
TSPoint readTouch(uint8_t wait) {
  TSPoint point = ts.getPoint();
  
  // Calibrate coordinates
  if (point.z > MINPRESSURE) {
    point.x = map(point.x, TS_MINX, TS_MAXX, 0, LCD_W);
    point.y = map(point.y, TS_MINY, TS_MAXY, LCD_H, 0);
  }
  
  return point;
}

/**
 * Find touch box ID from coordinates
 */
int findTouchIdFromCoords(uint16_t x, uint16_t y) {
  for (uint8_t i = 0; i < touchBoxCount; i++) {
    if (touchInBox(x, y, touchBoxes[i])) {
      return touchBoxes[i].id;
    }
  }
  return K_NONE;
}

/**
 * Add touch box
 */
void addTouchBox(uint16_t x, uint16_t y, uint8_t w, uint8_t h, int id) {
  if (touchBoxCount >= TOUCH_MAX) {
    return;
  }
  
  touchBoxes[touchBoxCount].x = x;
  touchBoxes[touchBoxCount].y = y;
  touchBoxes[touchBoxCount].w = w;
  touchBoxes[touchBoxCount].h = h;
  touchBoxes[touchBoxCount].id = id;
  touchBoxCount++;
}

// ============ SENSOR OPERATIONS ============

/**
 * Read all active sensors
 */
void readAllSensors() {
  memset(MODULES, 0, sizeof(MODULES));
  
  for (uint8_t i = 0; i < MaxSensors; i++) {
    Sensor* sensor = sensorManager.getSensor(i);
    if (sensor && sensor->active) {
      rfGetSensor(sensor->id, i);
      sensorManager.updateHistory(i);
    }
  }
}

/**
 * Request sensor value via RF24
 */
void rfGetSensor(uint16_t sensorId, uint8_t sensorIndex) {
  dataSend.cmd = CMD_GET;
  dataSend.id = sensorId;
  dataSend.val = 0;
  
  radio.powerUp();
  radio.stopListening();
  
  if (radio.write(&dataSend, sizeof(dataSend))) {
    if (rfRead()) {
      if (dataReceived.id == sensorId) {
        sensorManager.updateSensorValue(sensorIndex, dataReceived.val);
        MODULES[0]++;
      }
    }
  }
  
  radio.startListening();
  radio.powerDown();
}

/**
 * Read RF24 response
 */
bool rfRead() {
  uint32_t startTime = millis();
  
  radio.startListening();
  
  while (!radio.available() && (millis() - startTime) < RF24_TIMEOUT_MS) {
    delay(1);
  }
  
  if (!radio.available()) {
    return false;
  }
  
  radio.read(&dataReceived, sizeof(dataReceived));
  return true;
}

// ============ ALARM HANDLERS ============

/**
 * Alarm triggered callback
 */
void onSensorAlarmTriggered(uint8_t sensorIndex, int16_t value) {
  Serial.print(F("ALARM TRIGGERED: Sensor "));
  Serial.print(sensorIndex);
  Serial.print(F(" Value="));
  Serial.println(value);
  
  // Play alarm sound
  // sendSMSReport();
}

/**
 * Alarm cleared callback
 */
void onSensorAlarmCleared(uint8_t sensorIndex, int16_t value) {
  Serial.print(F("ALARM CLEARED: Sensor "));
  Serial.print(sensorIndex);
  Serial.print(F(" Value="));
  Serial.println(value);
}

// ============ GSM/SMS OPERATIONS ============

/**
 * Connect to GSM network
 */
void gsmConnect() {
  Serial.println(F("Connecting to GSM..."));
  // Implementation here
  gsm.connected = true;
}

/**
 * Send welcome report
 */
void sendWelcomeReport() {
  Serial.println(F("Sending welcome report..."));
  // Implementation here
}

// ============ END OF SKETCH ============
