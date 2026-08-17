#include <Arduino.h>
#include <Wire.h>
#include <math.h>
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
const int BUZZER_PIN = 7;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int OLED_ADDRESS = 0x3C;

const float TEMP_GOOD_ENTER_LOW = 19.0F;
const float TEMP_GOOD_ENTER_HIGH = 29.0F;
const float TEMP_GOOD_HOLD_LOW = 18.0F;
const float TEMP_GOOD_HOLD_HIGH = 30.0F;

const float TEMP_POOR_ENTER_LOW = 15.0F;
const float TEMP_POOR_ENTER_HIGH = 32.0F;
const float TEMP_POOR_RECOVER_LOW = 16.0F;
const float TEMP_POOR_RECOVER_HIGH = 31.0F;

const float HUMIDITY_GOOD_ENTER_LOW = 35.0F;
const float HUMIDITY_GOOD_ENTER_HIGH = 65.0F;
const float HUMIDITY_GOOD_HOLD_LOW = 30.0F;
const float HUMIDITY_GOOD_HOLD_HIGH = 70.0F;

const float HUMIDITY_POOR_ENTER_LOW = 20.0F;
const float HUMIDITY_POOR_ENTER_HIGH = 80.0F;
const float HUMIDITY_POOR_RECOVER_LOW = 25.0F;
const float HUMIDITY_POOR_RECOVER_HIGH = 75.0F;

const float LIGHT_GOOD_ENTER_MIN = 330.0F;
const float LIGHT_GOOD_HOLD_MIN = 300.0F;

const float LIGHT_POOR_ENTER_MIN = 50.0F;
const float LIGHT_POOR_RECOVER_MIN = 70.0F;

const float TEMP_SENSOR_MIN = -40.0F;
const float TEMP_SENSOR_MAX = 85.0F;
const float HUMIDITY_SENSOR_MIN = 0.0F;
const float HUMIDITY_SENSOR_MAX = 100.0F;
const float PRESSURE_SENSOR_MIN_HPA = 300.0F;
const float PRESSURE_SENSOR_MAX_HPA = 1100.0F;
const float LIGHT_SENSOR_MIN_LUX = 0.0F;

const unsigned long SENSOR_UPDATE_INTERVAL_MS = 2000;
const unsigned long BUZZER_DURATION_MS = 200;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET);

bool bmeReady = false;
bool bh1750Ready = false;
bool oledReady = false;

bool buzzerActive = false;

unsigned long lastSensorUpdateMs = 0;
unsigned long buzzerStartedAtMs = 0;

enum class EnvironmentStatus
{
  GOOD,
  WARNING,
  POOR
};

EnvironmentStatus currentStatus = EnvironmentStatus::WARNING;
EnvironmentStatus previousStatus = EnvironmentStatus::WARNING;

bool shouldEnterPoor(
    float temperature,
    float humidity,
    float lux)
{
  return temperature < TEMP_POOR_ENTER_LOW ||
         temperature > TEMP_POOR_ENTER_HIGH ||
         humidity < HUMIDITY_POOR_ENTER_LOW ||
         humidity > HUMIDITY_POOR_ENTER_HIGH ||
         lux < LIGHT_POOR_ENTER_MIN;
}

bool hasRecoveredFromPoor(
    float temperature,
    float humidity,
    float lux)
{
  return temperature >= TEMP_POOR_RECOVER_LOW &&
         temperature <= TEMP_POOR_RECOVER_HIGH &&
         humidity >= HUMIDITY_POOR_RECOVER_LOW &&
         humidity <= HUMIDITY_POOR_RECOVER_HIGH &&
         lux >= LIGHT_POOR_RECOVER_MIN;
}

bool shouldEnterGood(
    float temperature,
    float humidity,
    float lux)
{
  return temperature >= TEMP_GOOD_ENTER_LOW &&
         temperature <= TEMP_GOOD_ENTER_HIGH &&
         humidity >= HUMIDITY_GOOD_ENTER_LOW &&
         humidity <= HUMIDITY_GOOD_ENTER_HIGH &&
         lux >= LIGHT_GOOD_ENTER_MIN;
}

bool shouldRemainGood(
    float temperature,
    float humidity,
    float lux)
{
  return temperature >= TEMP_GOOD_HOLD_LOW &&
         temperature <= TEMP_GOOD_HOLD_HIGH &&
         humidity >= HUMIDITY_GOOD_HOLD_LOW &&
         humidity <= HUMIDITY_GOOD_HOLD_HIGH &&
         lux >= LIGHT_GOOD_HOLD_MIN;
}

EnvironmentStatus evaluateEnvironment(
    float temperature,
    float humidity,
    float lux,
    EnvironmentStatus previousEnvironmentStatus)
{
  if (!bmeReady || !bh1750Ready)
  {
    return EnvironmentStatus::WARNING;
  }

  if (previousEnvironmentStatus == EnvironmentStatus::POOR &&
      !hasRecoveredFromPoor(
          temperature,
          humidity,
          lux))
  {
    return EnvironmentStatus::POOR;
  }

  if (shouldEnterPoor(
          temperature,
          humidity,
          lux))
  {
    return EnvironmentStatus::POOR;
  }

  if (previousEnvironmentStatus == EnvironmentStatus::GOOD &&
      shouldRemainGood(
          temperature,
          humidity,
          lux))
  {
    return EnvironmentStatus::GOOD;
  }

  if (shouldEnterGood(
          temperature,
          humidity,
          lux))
  {
    return EnvironmentStatus::GOOD;
  }

  return EnvironmentStatus::WARNING;
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

bool isTemperatureValid(float temperature)
{
  return isfinite(temperature) &&
         temperature >= TEMP_SENSOR_MIN &&
         temperature <= TEMP_SENSOR_MAX;
}

bool isHumidityValid(float humidity)
{
  return isfinite(humidity) &&
         humidity >= HUMIDITY_SENSOR_MIN &&
         humidity <= HUMIDITY_SENSOR_MAX;
}

bool isPressureValid(float pressure)
{
  return isfinite(pressure) &&
         pressure >= PRESSURE_SENSOR_MIN_HPA &&
         pressure <= PRESSURE_SENSOR_MAX_HPA;
}

bool isLightValid(float lux)
{
  return isfinite(lux) &&
         lux >= LIGHT_SENSOR_MIN_LUX;
}

bool areSensorReadingsValid(
    float temperature,
    float humidity,
    float pressure,
    float lux)
{
  return bmeReady &&
         bh1750Ready &&
         isTemperatureValid(temperature) &&
         isHumidityValid(humidity) &&
         isPressureValid(pressure) &&
         isLightValid(lux);
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

void updateBuzzer(
    EnvironmentStatus status,
    unsigned long currentMillis)
{
  if (buzzerActive &&
      currentMillis - buzzerStartedAtMs >= BUZZER_DURATION_MS)
  {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  if (status != previousStatus)
  {
    if (status == EnvironmentStatus::POOR)
    {
      Serial.println("Alert: environment entered POOR state.");

      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStartedAtMs = currentMillis;
    }

    previousStatus = status;
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
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

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

  if (lightMeter.begin(
          BH1750::CONTINUOUS_HIGH_RES_MODE,
          0x23,
          &Wire))
  {
    bh1750Ready = true;
    Serial.println("BH1750 detected at address 0x23.");
  }
  else
  {
    Serial.println("BH1750 not detected.");
  }

  if (display.begin(
          SSD1306_SWITCHCAPVCC,
          OLED_ADDRESS))
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

  lastSensorUpdateMs =
      millis() - SENSOR_UPDATE_INTERVAL_MS;
}

void loop()
{
  unsigned long currentMillis = millis();

  updateBuzzer(
      currentStatus,
      currentMillis);

  if (currentMillis - lastSensorUpdateMs <
      SENSOR_UPDATE_INTERVAL_MS)
  {
    return;
  }

  lastSensorUpdateMs = currentMillis;

  float temperature = NAN;
  float humidity = NAN;
  float pressure = NAN;
  float lux = NAN;

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

  bool sensorDataValid =
      areSensorReadingsValid(
          temperature,
          humidity,
          pressure,
          lux);

  if (sensorDataValid)
  {
    Serial.println("Sensor validation: PASS");

    currentStatus =
        evaluateEnvironment(
            temperature,
            humidity,
            lux,
            currentStatus);
  }
  else
  {
    Serial.println("Sensor validation: FAILED");
    Serial.println("Fail-safe: forcing WARNING state.");

    currentStatus = EnvironmentStatus::WARNING;
  }

  updateStatusLeds(currentStatus);

  updateBuzzer(
      currentStatus,
      millis());

  Serial.print("Status: ");

  if (sensorDataValid)
  {
    Serial.println(statusToString(currentStatus));
  }
  else
  {
    Serial.println("WARNING (SENSOR ERROR)");
  }

  if (oledReady)
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("STATUS: ");

    if (sensorDataValid)
    {
      display.println(statusToString(currentStatus));
    }
    else
    {
      display.println("SENSOR ERROR");
    }

    display.setCursor(0, 14);
    display.print("Temp: ");

    if (bmeReady &&
        isTemperatureValid(temperature))
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

    if (bmeReady &&
        isHumidityValid(humidity))
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

    if (bh1750Ready &&
        isLightValid(lux))
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

    if (bmeReady &&
        isPressureValid(pressure))
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
}
