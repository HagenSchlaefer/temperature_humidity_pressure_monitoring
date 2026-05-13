/* filename: main.cpp */    
#include <Arduino.h>
#include <DHT.h>
#include "dht_sensor.h"

// Pin definitions
typedef enum {
  DHT_PIN = 10,
  BUTTON_PIN = 2,
  TEMP_LED_PIN = 13,
  HUMIDITY_LED_PIN = 12,
  ERROR_LED_PIN = 11
} PinId;

// DHT Sensor Setup
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Structures
// === PHYSISCHE INPUTS (only raw data) ===
typedef struct {
  float temperature;      // Rohdaten vom DHT11
  float humidity;         // Rohdaten vom DHT11
  int buttonValue;        // Rohdaten vom Button
} InputState;

// === INPUT CHANGES ===
typedef struct {
  bool temperatureChanged;
  bool humidityChanged;
  bool buttonChanged;
} InputDiff;

// === DERIVED STATES (with validation) ===
typedef struct {
  TempLevel tempLevel;
  HumidityLevel humidityLevel;
  bool sensorError;       // sensor error flag
} DerivedState;

// === DERIVED CHANGES ===
typedef struct {
  bool tempLevelChanged;
  bool humidityLevelChanged;
  bool sensorErrorChanged;
} DerivedDiff;

bool shouldReadSensor(unsigned long interval) {
  static unsigned long lastRead = 0;
  
  unsigned long now = millis();
  if (now - lastRead >= interval) {
    lastRead = now;
    return true;
  }
  return false;
}

//  read Input with 1Hz Timing
void readInput(InputState* state) {
  static unsigned long lastSensorRead = 0;  // HIER als static!
  const unsigned long DHT_INTERVAL = 1000;
  
  // read button 
  state->buttonValue = digitalRead(BUTTON_PIN);
  
  // read DHT11 every DHT_INTERVAL milliseconds
  if (shouldReadSensor(DHT_INTERVAL)) {
    state->temperature = dht.readTemperature();
    state->humidity = dht.readHumidity();
  }
}

InputDiff diffInput(const InputState* current, const InputState* last) {
  InputDiff diff = {0};
  
  // temperature change with tolerance (0.5°C)
  diff.temperatureChanged = abs(current->temperature - last->temperature) >= 0.5;
  
  // humidity change with tolerance (2%)
  diff.humidityChanged = abs(current->humidity - last->humidity) >= 2.0;
  
  diff.buttonChanged = current->buttonValue != last->buttonValue;
  
  return diff;
}

DerivedDiff diffDerived(const DerivedState* current, const DerivedState* last) {
  DerivedDiff diff = {0};
  
  diff.tempLevelChanged = current->tempLevel != last->tempLevel;
  diff.humidityLevelChanged = current->humidityLevel != last->humidityLevel;
  diff.sensorErrorChanged = current->sensorError != last->sensorError;
  
  return diff;
}

void handleSensorTransition(const DerivedState* current, const DerivedState* last) {
  // LED-control based on derived state
  
  if (current->sensorError) {
    digitalWrite(ERROR_LED_PIN, HIGH);
    digitalWrite(TEMP_LED_PIN, LOW);
    digitalWrite(HUMIDITY_LED_PIN, LOW);
    return;
  } else {
    digitalWrite(ERROR_LED_PIN, LOW);
  }
  
  // temperatur LED
  digitalWrite(TEMP_LED_PIN, (current->tempLevel == TEMP_HOT) ? HIGH : LOW);
  
  // humaidity LED
  digitalWrite(HUMIDITY_LED_PIN, (current->humidityLevel == HUMIDITY_HIGH) ? HIGH : LOW);
}

void setup() {
  // pin setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEMP_LED_PIN, OUTPUT);
  pinMode(HUMIDITY_LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  
  // led initial state
  digitalWrite(TEMP_LED_PIN, LOW);
  digitalWrite(HUMIDITY_LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  
  // DHT11 init
  dht.begin();
  
  // Serial init
  Serial.begin(9600);
  Serial.println("DHT11 Monitoring gestartet...");
}

void loop() {
  //constants for thresholds and valid ranges
  const float TEMP_COLD_THRESHOLD = 18.0;
  const float TEMP_NORMAL_THRESHOLD = 25.0;
  const float TEMP_WARM_THRESHOLD = 30.0;
  const float TEMP_MIN_VALID = 0.0;
  const float TEMP_MAX_VALID = 50.0;
  const float HUMIDITY_LOW_THRESHOLD = 40.0;
  const float HUMIDITY_NORMAL_THRESHOLD = 60.0;
  const float HUMIDITY_MIN_VALID = 20.0;
  const float HUMIDITY_MAX_VALID = 90.0;

  static InputState currentInputState, lastInputState;
  static DerivedState currentDerivedState, lastDerivedState;
  static bool derivedInitialized = false;
  
  // read the current input values
  readInput(&currentInputState);
  
  // detect input changes
  InputDiff inputDiff = diffInput(&currentInputState, &lastInputState);
  
  // button-handling
  if (inputDiff.buttonChanged && currentInputState.buttonValue == LOW) {
    Serial.println("Reset gedrückt!");
    derivedInitialized = false;
    delay(100);
  }
  
  // initialization of derived state on first run or after reset
  if (!derivedInitialized) {
    lastDerivedState.tempLevel = TEMP_UNKNOWN;
    lastDerivedState.humidityLevel = HUMIDITY_UNKNOWN;
    derivedInitialized = true;
  }

  // Derive states with validation
  bool currentSensorValid = !isnan(currentInputState.temperature) && !isnan(currentInputState.humidity);
  if (currentSensorValid) {
    currentDerivedState.tempLevel = getTempLevel(currentInputState.temperature, TEMP_COLD_THRESHOLD, TEMP_NORMAL_THRESHOLD, TEMP_WARM_THRESHOLD, TEMP_MIN_VALID, TEMP_MAX_VALID);
    currentDerivedState.humidityLevel = getHumidityLevel(currentInputState.humidity, HUMIDITY_LOW_THRESHOLD, HUMIDITY_NORMAL_THRESHOLD, HUMIDITY_MIN_VALID, HUMIDITY_MAX_VALID);
    currentDerivedState.sensorError = false;
  } else {
    currentDerivedState.tempLevel = TEMP_UNKNOWN;
    currentDerivedState.humidityLevel = HUMIDITY_UNKNOWN;
    currentDerivedState.sensorError = true;
  }
  
  // detect derived state changes
  DerivedDiff derivedDiff = diffDerived(&currentDerivedState, &lastDerivedState);
  
  // only handle transitions if there are changes in derived state
  if (derivedDiff.tempLevelChanged || derivedDiff.humidityLevelChanged || derivedDiff.sensorErrorChanged) {
    
    handleSensorTransition(&currentDerivedState, &lastDerivedState);
    
    // Serial output
    Serial.print("Temp: ");
    Serial.print(currentInputState.temperature);
    Serial.print("°C (Level: ");
    Serial.print(currentDerivedState.tempLevel);
    Serial.print(") | Humidity: ");
    Serial.print(currentInputState.humidity);
    Serial.print("% (Level: ");
    Serial.print(currentDerivedState.humidityLevel);
    Serial.println(")");
  }
  
  // save current states for next loop iteration
  lastDerivedState = currentDerivedState;
  lastInputState = currentInputState;
  
  // small delay to avoid overwhelming the serial output and allow for sensor stabilization
  delay(50);
}
