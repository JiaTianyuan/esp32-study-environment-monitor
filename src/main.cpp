#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
bool bmeReady = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Environment Monitor");

  Wire.begin();

  if (bme.begin(0x76, &Wire)) {
    bmeReady = true;
    Serial.println("BME280 detected at address 0x76.");
  } else if (bme.begin(0x77, &Wire)) {
    bmeReady = true;
    Serial.println("BME280 detected at address 0x77.");
  } else {
    Serial.println("BME280 not detected.");
    Serial.println("Connect the sensor, then restart the ESP32.");
  }
}

void loop() {
  if (!bmeReady) {
    Serial.println("Waiting for BME280...");
    delay(2000);
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(bme.readTemperature());
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.println();
  delay(2000);
}