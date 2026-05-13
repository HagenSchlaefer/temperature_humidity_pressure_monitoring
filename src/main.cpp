    
#include <Arduino.h>
#include <DHT.h>
#include "dht_sensor.h"

// Pin definitions
typedef enum {
  DHT_PIN = 10,
  BUTTON_PIN = 3,
  TEMP_LED_PIN = 13,
  HUMIDITY_LED_PIN = 12,
  ERROR_LED_PIN = 11
} PinId;

// DHT Sensor Setup
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Structures
typedef struct {
  float temperature;
  float humidity;
  bool sensorValid;
  int buttonValue;
  unsigned long lastSensorRead;  // Timing für 1Hz Sampling
} InputState;

typedef struct {
  bool temperatureChanged;
  bool humidityChanged;
  bool buttonChanged;
  bool sensorValidChanged;
} InputDiff;

typedef struct {
  TempLevel tempLevel;
  HumidityLevel humidityLevel;
  bool sensorError;
} DerivedState;

typedef struct {
  bool tempLevelChanged;
  bool humidityLevelChanged;
  bool sensorErrorChanged;
} DerivedDiff;

// **SCHLÜSSEL-FUNKTION: Smart readInput mit 1Hz Timing**
void readInput(InputState* state) {
  const unsigned long DHT_INTERVAL = 1000; // 1Hz = 1000ms
  
  // Button immer lesen (schnell)
  state->buttonValue = digitalRead(BUTTON_PIN);
  
  // DHT11 nur alle 1000ms lesen
  unsigned long currentTime = millis();
  if (currentTime - state->lastSensorRead >= DHT_INTERVAL) {
    
    float newTemp = dht.readTemperature();
    float newHumidity = dht.readHumidity();
    
    if (isnan(newTemp) || isnan(newHumidity)) {
      state->sensorValid = false;
      // Behalte letzte gültige Werte bei
    } else {
      state->temperature = newTemp;
      state->humidity = newHumidity;
      state->sensorValid = true;
    }
    
    state->lastSensorRead = currentTime;
  }
  // Wenn noch nicht Zeit für neuen Read: behalte alte Werte
}

InputDiff diffInput(const InputState* current, const InputState* last) {
  InputDiff diff = {0};
  
  // Temperatur-Änderung mit Toleranz (0.5°C)
  diff.temperatureChanged = abs(current->temperature - last->temperature) >= 0.5;
  
  // Luftfeuchtigkeit-Änderung mit Toleranz (2%)
  diff.humidityChanged = abs(current->humidity - last->humidity) >= 2.0;
  
  diff.buttonChanged = current->buttonValue != last->buttonValue;
  diff.sensorValidChanged = current->sensorValid != last->sensorValid;
  
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
  // LED-Steuerung basierend auf Zustandsänderungen
  
  if (current->sensorError) {
    digitalWrite(ERROR_LED_PIN, HIGH);
    digitalWrite(TEMP_LED_PIN, LOW);
    digitalWrite(HUMIDITY_LED_PIN, LOW);
    return;
  } else {
    digitalWrite(ERROR_LED_PIN, LOW);
  }
  
  // Temperatur LED
  digitalWrite(TEMP_LED_PIN, (current->tempLevel == TEMP_HOT) ? HIGH : LOW);
  
  // Luftfeuchtigkeit LED
  digitalWrite(HUMIDITY_LED_PIN, (current->humidityLevel == HUMIDITY_HIGH) ? HIGH : LOW);
}

void setup() {
  // Pins konfigurieren
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEMP_LED_PIN, OUTPUT);
  pinMode(HUMIDITY_LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  
  // LEDs initial ausschalten
  digitalWrite(TEMP_LED_PIN, LOW);
  digitalWrite(HUMIDITY_LED_PIN, LOW);
  digitalWrite(ERROR_LED_PIN, LOW);
  
  // DHT11 initialisieren
  dht.begin();
  
  Serial.begin(9600);
  Serial.println("DHT11 Monitoring gestartet...");
}

void loop() {
  const float TEMP_COLD_THRESHOLD = 18.0;
  const float TEMP_NORMAL_THRESHOLD = 25.0;
  const float TEMP_WARM_THRESHOLD = 30.0;
  const float TEMP_MIN_VALID = 0.0;
  const float TEMP_MAX_VALID = 50.0;
  const float HUMIDITY_LOW_THRESHOLD = 40.0;
  const float HUMIDITY_NORMAL_THRESHOLD = 60.0;
  const float HUMIDITY_MIN_VALID = 20.0;
  const float HUMIDITY_MAX_VALID = 90.0;

  static InputState currentInputState = {0};
  static InputState lastInputState = {0};
  static DerivedState currentDerivedState = {0};
  static DerivedState lastDerivedState = {0};
  static bool derivedInitialized = false;
  
  // **Inputs lesen (mit intelligentem DHT-Timing)**
  readInput(&currentInputState);
  
  // Input-Änderungen erkennen
  InputDiff inputDiff = diffInput(&currentInputState, &lastInputState);
  
  // Button-Handling
  if (inputDiff.buttonChanged && currentInputState.buttonValue == LOW) {
    Serial.println("Reset gedrückt!");
    derivedInitialized = false;
    delay(100);
  }
  
  // **Derived State berechnen**
  if (!derivedInitialized) {
    lastDerivedState.tempLevel = TEMP_UNKNOWN;
    lastDerivedState.humidityLevel = HUMIDITY_UNKNOWN;
    derivedInitialized = true;
  }
  
  currentDerivedState.tempLevel = getTempLevel(currentInputState.temperature, TEMP_COLD_THRESHOLD, TEMP_NORMAL_THRESHOLD, TEMP_WARM_THRESHOLD, TEMP_MIN_VALID, TEMP_MAX_VALID);
  currentDerivedState.humidityLevel = getHumidityLevel(currentInputState.humidity, HUMIDITY_LOW_THRESHOLD, HUMIDITY_NORMAL_THRESHOLD, HUMIDITY_MIN_VALID, HUMIDITY_MAX_VALID);
  currentDerivedState.sensorError = !currentInputState.sensorValid;
  
  // Derived-Änderungen erkennen
  DerivedDiff derivedDiff = diffDerived(&currentDerivedState, &lastDerivedState);
  
  // **NUR bei Änderungen ausgeben und LEDs steuern**
  if (derivedDiff.tempLevelChanged || derivedDiff.humidityLevelChanged || derivedDiff.sensorErrorChanged) {
    
    handleSensorTransition(&currentDerivedState, &lastDerivedState);
    
    // **Ausgabe nur bei Änderungen**
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
  
  // States für nächste Iteration speichern
  lastDerivedState = currentDerivedState;
  lastInputState = currentInputState;
  
  // Kleine Pause für Loop
  delay(50);
}
