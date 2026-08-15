#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeReady = false;
bool bh1750Ready = false;

const int SDA_PIN = 8;
const int SCL_PIN = 9;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Study Environment Monitor");

  Wire.begin(SDA_PIN, SCL_PIN);

  if (bme.begin(0x76, &Wire))
  {
    bmeReady = true;
    Serial.println("BME280 detected at address 0x76.");
  }
  else if (bme.begin(0x77, &Wire))
  {
    bmeReady = true;
    Serial.println("BME280 detected at address 0x77.");
  }
  else
  {
    Serial.println("BME280 not detected.");
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire))
  {
    bh1750Ready = true;
    Serial.println("BH1750 detected at address 0x23.");
  }
  else
  {
    Serial.println("BH1750 not detected.");
  }
}

void loop()
{
  if (bmeReady)
  {
    Serial.print("Temperature: ");
    Serial.print(bme.readTemperature());
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");
  }
  else
  {
    Serial.println("BME280 unavailable.");
  }

  if (bh1750Ready)
  {
    float lux = lightMeter.readLightLevel();

    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  }
  else
  {
    Serial.println("BH1750 unavailable.");
  }

  Serial.println();
  delay(2000);
}