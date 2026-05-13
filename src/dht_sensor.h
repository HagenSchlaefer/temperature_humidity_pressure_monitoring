#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

typedef enum {
  TEMP_UNKNOWN = -1,
  TEMP_COLD = 0,     // < 18°C
  TEMP_NORMAL = 1,   // 18-25°C
  TEMP_WARM = 2,     // 25-30°C
  TEMP_HOT = 3       // > 30°C
} TempLevel;

typedef enum {
  HUMIDITY_UNKNOWN = -1,
  HUMIDITY_LOW = 0,     // < 40%
  HUMIDITY_NORMAL = 1,  // 40-60%
  HUMIDITY_HIGH = 2     // > 60%
} HumidityLevel;

// Function prototypes
TempLevel getTempLevel(float temperature, float coldThreshold, float normalThreshold, float warmThreshold, float minValidTemp, float maxValidTemp);
HumidityLevel getHumidityLevel(float humidity, float lowHumidityThreshold, float normalHumidityThreshold, float minValidHumidity, float maxValidHumidity);

#endif