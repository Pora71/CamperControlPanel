/*
 * AutoCamp State Machine Manager
 * Handles screen transitions, events, and application state
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "config.h"
#include "types.h"
#include "utils.h"

// Forward declarations
class StateMachine;

// ============ STATE HANDLER CALLBACKS ============

/**
 * Screen state handler function pointer
 * Returns true if state should continue, false if should transition
 */
typedef bool (*ScreenStateHandler)(StateMachine* sm, int buttonId);

/**
 * Event handler for application events
 */
typedef void (*EventHandler)(StateMachine* sm, uint8_t eventType, void* data);

// ============ EVENT TYPES ============

enum EventType {
  EVENT_TOUCH = 0,
  EVENT_SENSOR_READ = 1,
  EVENT_TIMER = 2,
  EVENT_ERROR = 3,
  EVENT_RF_DATA = 4,
  EVENT_ENCODER = 5,
  EVENT_GSM = 6,
  EVENT_ALARM = 7
};

// ============ STATE MACHINE CLASS ============

class StateMachine {
private:
  ScreenState currentState;
  ScreenState previousState;
  uint32_t stateEnterTime;
  uint32_t stateTimeout;
  bool stateNeedsRedraw;
  
  // Screen handler function pointers
  ScreenStateHandler screenHandlers[11];
  
  // Event handlers
  EventHandler eventHandlers[8];
  
  // Application state
  uint8_t currentPage;
  uint8_t selectedSensor;
  bool isTransitioning;
  
  // Queued events
  struct QueuedEvent {
    EventType type;
    int data;
    uint32_t timestamp;
  };
  
  static const uint8_t MAX_QUEUED_EVENTS = 10;
  QueuedEvent eventQueue[MAX_QUEUED_EVENTS];
  uint8_t eventQueueCount;
  
public:
  
  // ============ INITIALIZATION ============
  
  /**
   * Initialize state machine
   */
  StateMachine() : currentState(SCREEN_MAIN), previousState(SCREEN_MAIN),
                   stateEnterTime(0), stateTimeout(0), stateNeedsRedraw(true),
                   currentPage(0), selectedSensor(0), isTransitioning(false),
                   eventQueueCount(0) {
    // Initialize all handlers to nullptr
    for (uint8_t i = 0; i < 11; i++) {
      screenHandlers[i] = nullptr;
    }
    for (uint8_t i = 0; i < 8; i++) {
      eventHandlers[i] = nullptr;
    }
  }
  
  /**
   * Register screen state handler
   */
  void registerScreenHandler(ScreenState state, ScreenStateHandler handler) {
    if (state < 11) {
      screenHandlers[state] = handler;
    }
  }
  
  /**
   * Register event handler
   */
  void registerEventHandler(EventType eventType, EventHandler handler) {
    if (eventType < 8) {
      eventHandlers[eventType] = handler;
    }
  }
  
  // ============ STATE MANAGEMENT ============
  
  /**
   * Transition to new state
   */
  void transitionTo(ScreenState newState) {
    if (newState == currentState) {
      return;  // Already in this state
    }
    
    previousState = currentState;
    currentState = newState;
    stateEnterTime = millis();
    stateTimeout = 0;
    stateNeedsRedraw = true;
    isTransitioning = true;
    
    #ifdef DEBUG
      Serial.print("State transition: ");
      Serial.print(previousState);
      Serial.print(" -> ");
      Serial.println(currentState);
    #endif
  }
  
  /**
   * Return to previous state
   */
  void transitionBack() {
    ScreenState temp = currentState;
    currentState = previousState;
    previousState = temp;
    stateEnterTime = millis();
    stateNeedsRedraw = true;
    isTransitioning = true;
  }
  
  /**
   * Get current state
   */
  ScreenState getCurrentState() const {
    return currentState;
  }
  
  /**
   * Get previous state
   */
  ScreenState getPreviousState() const {
    return previousState;
  }
  
  /**
   * Check if state needs redraw
   */
  bool needsRedraw() const {
    return stateNeedsRedraw;
  }
  
  /**
   * Mark state as redrawn
   */
  void markRedrawn() {
    stateNeedsRedraw = false;
    isTransitioning = false;
  }
  
  /**
   * Set state redraw flag
   */
  void setNeedsRedraw(bool needs) {
    stateNeedsRedraw = needs;
  }
  
  /**
   * Set state timeout (for auto-transition)
   */
  void setStateTimeout(uint32_t timeoutMs) {
    stateTimeout = timeoutMs;
  }
  
  /**
   * Check if state timeout has elapsed
   */
  bool isStateTimeout() const {
    if (stateTimeout == 0) return false;
    return (millis() - stateEnterTime) > stateTimeout;
  }
  
  /**
   * Get time in current state
   */
  uint32_t getTimeInState() const {
    return millis() - stateEnterTime;
  }
  
  // ============ PAGE MANAGEMENT ============
  
  /**
   * Set current page (for multi-page screens)
   */
  void setPage(uint8_t page) {
    currentPage = page;
    stateNeedsRedraw = true;
  }
  
  /**
   * Get current page
   */
  uint8_t getPage() const {
    return currentPage;
  }
  
  /**
   * Advance to next page with wrap-around
   */
  void nextPage(uint8_t maxPages) {
    currentPage++;
    if (currentPage >= maxPages) {
      currentPage = 0;
    }
    stateNeedsRedraw = true;
  }
  
  /**
   * Go to previous page with wrap-around
   */
  void prevPage(uint8_t maxPages) {
    if (currentPage == 0) {
      currentPage = maxPages - 1;
    } else {
      currentPage--;
    }
    stateNeedsRedraw = true;
  }
  
  // ============ SENSOR SELECTION ============
  
  /**
   * Set selected sensor
   */
  void setSelectedSensor(uint8_t sensorIndex) {
    selectedSensor = sensorIndex;
    stateNeedsRedraw = true;
  }
  
  /**
   * Get selected sensor
   */
  uint8_t getSelectedSensor() const {
    return selectedSensor;
  }
  
  // ============ EVENT HANDLING ============
  
  /**
   * Queue event for processing
   */
  bool queueEvent(EventType eventType, int eventData) {
    if (eventQueueCount >= MAX_QUEUED_EVENTS) {
      #ifdef DEBUG
        Serial.println("Event queue full!");
      #endif
      return false;
    }
    
    eventQueue[eventQueueCount].type = eventType;
    eventQueue[eventQueueCount].data = eventData;
    eventQueue[eventQueueCount].timestamp = millis();
    eventQueueCount++;
    
    return true;
  }
  
  /**
   * Process queued events
   */
  void processEvents() {
    for (uint8_t i = 0; i < eventQueueCount; i++) {
      const QueuedEvent& evt = eventQueue[i];
      
      if (eventHandlers[evt.type] != nullptr) {
        eventHandlers[evt.type](this, evt.type, (void*)&evt.data);
      }
    }
    
    eventQueueCount = 0;
  }
  
  /**
   * Handle button press
   */
  bool handleButtonPress(int buttonId) {
    if (screenHandlers[currentState] != nullptr) {
      return screenHandlers[currentState](this, buttonId);
    }
    return false;
  }
  
  /**
   * Get event queue count
   */
  uint8_t getEventQueueCount() const {
    return eventQueueCount;
  }
  
  // ============ UTILITY FUNCTIONS ============
  
  /**
   * Clear all queued events
   */
  void clearEventQueue() {
    eventQueueCount = 0;
  }
  
  /**
   * Reset state machine to initial state
   */
  void reset() {
    currentState = SCREEN_MAIN;
    previousState = SCREEN_MAIN;
    stateEnterTime = millis();
    stateTimeout = 0;
    stateNeedsRedraw = true;
    currentPage = 0;
    selectedSensor = 0;
    isTransitioning = false;
    eventQueueCount = 0;
  }
  
  /**
   * Check if currently transitioning
   */
  bool isInTransition() const {
    return isTransitioning;
  }
  
  /**
   * Get state name for debugging
   */
  const char* getStateName(ScreenState state) const {
    switch (state) {
      case SCREEN_MAIN: return "MAIN";
      case SCREEN_DEVICE: return "DEVICE";
      case SCREEN_SETUP: return "SETUP";
      case SCREEN_GRAPH: return "GRAPH";
      case SCREEN_SOUND: return "SOUND";
      case SCREEN_SENSORS: return "SENSORS";
      case SCREEN_SENSOR: return "SENSOR";
      case SCREEN_ICONS: return "ICONS";
      case SCREEN_MODEM: return "MODEM";
      case SCREEN_DATETIME: return "DATETIME";
      default: return "UNKNOWN";
    }
  }
};

// ============ SCREEN STATE HANDLERS ============

/**
 * Main screen state handler
 * Handles: K_SETUP, K_PAGE, sensor buttons
 */
bool handleMainScreen(StateMachine* sm, int buttonId);

/**
 * Setup screen state handler
 * Handles: K_SENSORS, K_SOUND, K_MODEM, K_DATETIME, K_CANCEL
 */
bool handleSetupScreen(StateMachine* sm, int buttonId);

/**
 * Sensors configuration screen handler
 * Handles: sensor selection, K_DEFAULT, K_EXIT
 */
bool handleSensorsScreen(StateMachine* sm, int buttonId);

/**
 * Single sensor configuration handler
 * Handles: K_ID, K_NAME, K_ICON, K_MIN, K_MAX, K_ALARM, K_SAVE
 */
bool handleSensorScreen(StateMachine* sm, int buttonId);

/**
 * Graph display handler
 * Handles: K_MENU, K_SETUP, K_24H, K_1H, K_ON, K_OFF, K_RANDOM
 */
bool handleGraphScreen(StateMachine* sm, int buttonId);

/**
 * Modem configuration handler
 * Handles: K_TEST, K_VERSION, K_SMS, K_RESET, K_NET, K_EXIT
 */
bool handleModemScreen(StateMachine* sm, int buttonId);

/**
 * Date/Time configuration handler
 * Handles: date/time increment/decrement buttons, K_SAVE, K_EXIT
 */
bool handleDateTimeScreen(StateMachine* sm, int buttonId);

// ============ EVENT HANDLERS ============

/**
 * Touch event handler
 */
void handleTouchEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * Sensor read event handler
 */
void handleSensorReadEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * Timer event handler
 */
void handleTimerEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * Error event handler
 */
void handleErrorEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * RF data event handler
 */
void handleRFDataEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * Encoder event handler
 */
void handleEncoderEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * GSM event handler
 */
void handleGSMEvent(StateMachine* sm, uint8_t eventType, void* data);

/**
 * Alarm event handler
 */
void handleAlarmEvent(StateMachine* sm, uint8_t eventType, void* data);

// ============ FACTORY FUNCTION ============

/**
 * Create and initialize state machine
 */
StateMachine* createStateMachine() {
  StateMachine* sm = new StateMachine();
  
  // Register screen handlers
  sm->registerScreenHandler(SCREEN_MAIN, handleMainScreen);
  sm->registerScreenHandler(SCREEN_SETUP, handleSetupScreen);
  sm->registerScreenHandler(SCREEN_SENSORS, handleSensorsScreen);
  sm->registerScreenHandler(SCREEN_SENSOR, handleSensorScreen);
  sm->registerScreenHandler(SCREEN_GRAPH, handleGraphScreen);
  sm->registerScreenHandler(SCREEN_MODEM, handleModemScreen);
  sm->registerScreenHandler(SCREEN_DATETIME, handleDateTimeScreen);
  
  // Register event handlers
  sm->registerEventHandler(EVENT_TOUCH, handleTouchEvent);
  sm->registerEventHandler(EVENT_SENSOR_READ, handleSensorReadEvent);
  sm->registerEventHandler(EVENT_TIMER, handleTimerEvent);
  sm->registerEventHandler(EVENT_ERROR, handleErrorEvent);
  sm->registerEventHandler(EVENT_RF_DATA, handleRFDataEvent);
  sm->registerEventHandler(EVENT_ENCODER, handleEncoderEvent);
  sm->registerEventHandler(EVENT_GSM, handleGSMEvent);
  sm->registerEventHandler(EVENT_ALARM, handleAlarmEvent);
  
  return sm;
}

#endif // STATE_MACHINE_H
