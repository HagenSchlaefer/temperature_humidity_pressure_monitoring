#include "dht_sensor.h"

TempLevel getTempLevel(float temperature, float coldThreshold, float normalThreshold, float warmThreshold, float minValidTemp, float maxValidTemp) {
    if(minValidTemp >= coldThreshold || coldThreshold >= normalThreshold || normalThreshold >= warmThreshold || warmThreshold >= maxValidTemp) return TEMP_UNKNOWN;
    if(temperature < minValidTemp || temperature > maxValidTemp) return TEMP_UNKNOWN;

    if(temperature < coldThreshold) return TEMP_COLD;
    else if(temperature < normalThreshold) return TEMP_NORMAL;
    else if(temperature < warmThreshold) return TEMP_WARM;
    else return TEMP_HOT;
}

HumidityLevel getHumidityLevel(float humidity, float lowHumidityThreshold, float normalHumidityThreshold, float minValidHumidity, float maxValidHumidity) {
    if(minValidHumidity >= lowHumidityThreshold || lowHumidityThreshold >= normalHumidityThreshold || normalHumidityThreshold >= maxValidHumidity) return HUMIDITY_UNKNOWN;
    if(humidity < minValidHumidity || humidity > maxValidHumidity) return HUMIDITY_UNKNOWN;

    if(humidity < lowHumidityThreshold) return HUMIDITY_LOW;
    else if(humidity <= normalHumidityThreshold) return HUMIDITY_NORMAL;
    else return HUMIDITY_HIGH;
}