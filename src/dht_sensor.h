/* filename: dht_sensor.h */
#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

typedef enum {
  TEMP_UNKNOWN = -1,
  TEMP_COLD = 0,     
  TEMP_NORMAL = 1,   
  TEMP_WARM = 2,     
  TEMP_HOT = 3       
} TempLevel;

typedef enum {
  HUMIDITY_UNKNOWN = -1,
  HUMIDITY_LOW = 0,     
  HUMIDITY_NORMAL = 1,  
  HUMIDITY_HIGH = 2     
} HumidityLevel;

// Function prototypes
TempLevel getTempLevel(float temperature, float coldThreshold, float normalThreshold, float warmThreshold, float minValidTemp, float maxValidTemp);
HumidityLevel getHumidityLevel(float humidity, float lowHumidityThreshold, float normalHumidityThreshold, float minValidHumidity, float maxValidHumidity);

#endif