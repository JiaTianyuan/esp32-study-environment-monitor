#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

const int SDA_PIN = 8;
const int SCL_PIN = 9;

const int GREEN_LED_PIN = 4;
const int YELLOW_LED_PIN = 5;
const int RED_LED_PIN = 6;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int OLED_ADDRESS = 0x3C;

const float TEMP_WARNING_LOW = 18.0F;
const float TEMP_WARNING_HIGH = 30.0F;
const float TEMP_POOR_LOW = 15.0F;
const float TEMP_POOR_HIGH = 32.0F;

const float HUMIDITY_WARNING_LOW = 30.0F;
const float HUMIDITY_WARNING_HIGH = 70.0F;
const float HUMIDITY_POOR_LOW = 20.0F;
const float HUMIDITY_POOR_HIGH = 80.0F;

const float LIGHT_WARNING_MIN = 300.0F;
const float LIGHT_POOR_MIN = 50.0F;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET);

bool bmeReady = false;
bool bh1750Ready = false;
bool oledReady = false;

enum class EnvironmentStatus
{
  GOOD,
  WARNING,
  POOR
};

EnvironmentStatus evaluateEnvironment(
    float temperature,
    float humidity,
    float lux)
{
  if (!bmeReady || !bh1750Ready)
  {
    return EnvironmentStatus::WARNING;
  }

  bool poor =
      temperature < TEMP_POOR_LOW ||
      temperature > TEMP_POOR_HIGH ||
      humidity < HUMIDITY_POOR_LOW ||
      humidity > HUMIDITY_POOR_HIGH ||
      lux < LIGHT_POOR_MIN;

  if (poor)
  {
    return EnvironmentStatus::POOR;
  }

  bool warning =
      temperature < TEMP_WARNING_LOW ||
      temperature > TEMP_WARNING_HIGH ||
      humidity < HUMIDITY_WARNING_LOW ||
      humidity > HUMIDITY_WARNING_HIGH ||
      lux < LIGHT_WARNING_MIN;

  if (warning)
  {
    return EnvironmentStatus::WARNING;
  }

  return EnvironmentStatus::GOOD;
}

const char *statusToString(EnvironmentStatus status)
{
  switch (status)
  {
    case EnvironmentStatus::GOOD:
      return "GOOD";

    case EnvironmentStatus::WARNING:
      return "WARNING";

    case EnvironmentStatus::POOR:
      return "POOR";
  }

  return "UNKNOWN";
}

void updateStatusLeds(EnvironmentStatus status)
{
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  switch (status)
  {
    case EnvironmentStatus::GOOD:
      digitalWrite(GREEN_LED_PIN, HIGH);
      break;

    case EnvironmentStatus::WARNING:
      digitalWrite(YELLOW_LED_PIN, HIGH);
      break;

    case EnvironmentStatus::POOR:
      digitalWrite(RED_LED_PIN, HIGH);
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Study Environment Monitor");

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

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

  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    oledReady = true;
    Serial.println("OLED detected at address 0x3C.");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Study Environment");
    display.println("Monitor starting...");
    display.display();
  }
  else
  {
    Serial.println("OLED initialization failed.");
  }
}

void loop()
{
  float temperature = 0.0F;
  float humidity = 0.0F;
  float pressure = 0.0F;
  float lux = 0.0F;

  if (bmeReady)
  {
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
  }
  else
  {
    Serial.println("BME280 unavailable.");
  }

  if (bh1750Ready)
  {
    lux = lightMeter.readLightLevel();

    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  }
  else
  {
    Serial.println("BH1750 unavailable.");
  }

  EnvironmentStatus status =
      evaluateEnvironment(temperature, humidity, lux);

  updateStatusLeds(status);

  Serial.print("Status: ");
  Serial.println(statusToString(status));

  if (oledReady)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("STATUS: ");
    display.println(statusToString(status));

    display.setCursor(0, 14);
    display.print("Temp: ");
    if (bmeReady)
    {
      display.print(temperature, 1);
      display.println(" C");
    }
    else
    {
      display.println("--");
    }

    display.setCursor(0, 26);
    display.print("Hum : ");
    if (bmeReady)
    {
      display.print(humidity, 1);
      display.println(" %");
    }
    else
    {
      display.println("--");
    }

    display.setCursor(0, 38);
    display.print("Light: ");
    if (bh1750Ready)
    {
      display.print(lux, 1);
      display.println(" lx");
    }
    else
    {
      display.println("--");
    }

    display.setCursor(0, 50);
    display.print("Pres: ");
    if (bmeReady)
    {
      display.print(pressure, 1);
      display.println(" hPa");
    }
    else
    {
      display.println("--");
    }

    display.display();
  }

  Serial.println();
  delay(2000);
}
