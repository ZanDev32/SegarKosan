#ifndef DHT22_H
#define DHT22_H

#include <DHT.h>

// Default DHT22 pin for ESP32-C3 setup
#ifndef DHT_PIN
#define DHT_PIN 4
#endif
#define DHTTYPE DHT22

class DHT22Sensor {
private:
  DHT dht;

public:
  DHT22Sensor() : dht(DHT_PIN, DHTTYPE) {}

  void begin() {
    dht.begin();
  }

  float readHumidity() {
    return dht.readHumidity();
  }

  float readTemperature() {
    return dht.readTemperature();
  }

  static float computeHeatIndex(float temperature, float humidity) {
    return -8.784695 + 1.61139411*temperature + 2.338549*humidity
         - 0.14611605*temperature*humidity - 0.01230809*temperature*temperature 
         - 0.01642482*humidity*humidity + 0.00221173*temperature*temperature*humidity 
         + 0.00072546*temperature*humidity*humidity 
         - 0.00000358*temperature*temperature*humidity*humidity;
  }

  bool isValidReading(float temperature, float humidity) {
    return !isnan(temperature) && !isnan(humidity);
  }
};

#endif
