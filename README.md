# ESP32 Study Environment Monitor

An ESP32-S3 embedded system that monitors **temperature, humidity, atmospheric pressure, and ambient light**, then converts sensor data into an immediate **GOOD / WARNING / POOR** study-environment status.

The system combines multiple I²C sensors, an OLED dashboard, three status LEDs, and a transition-based buzzer alert into one working physical prototype.

![Working ESP32 Study Environment Monitor](docs/images/hardware-warning-state.jpg)

## Project Overview

Instead of only displaying raw environmental measurements, this project converts sensor data into actionable feedback.

The ESP32 continuously reads the environment, classifies the current conditions, updates the OLED display, changes the active status LED, and generates a short audible alert when the environment transitions into a POOR state.

| Feature | Implementation |
|---|---|
| Temperature | BME280 |
| Humidity | BME280 |
| Atmospheric pressure | BME280 |
| Ambient light | BH1750 / GY-302 |
| Local dashboard | 128×64 SSD1306 OLED |
| Visual feedback | Green / Yellow / Red LEDs |
| Audible feedback | Active buzzer module |
| Controller | ESP32-S3 |
| Firmware | Arduino C++ |
| Build system | PlatformIO |
| Communication | Shared I²C bus + Serial |

## System Architecture

```text
        BME280
  Temp / Hum / Pressure
            |
            |
            v
         ESP32-S3
            ^
            |
            |
         BH1750
       Ambient Light

            |
            v

   evaluateEnvironment()

            |
     +------+------+
     |      |      |
     v      v      v
   GOOD   WARNING  POOR
     |      |      |
   Green  Yellow   Red
    LED     LED    LED
                   +
               Buzzer Alert

            |
            v

      SSD1306 OLED
      Serial Monitor
```

Three devices share the same I²C bus:

```text
BH1750       -> 0x23
SSD1306 OLED -> 0x3C
BME280       -> 0x76
```

The firmware also attempts `0x77` as a fallback BME280 address.

## Environment Classification

The current firmware uses configurable prototype thresholds.

These thresholds were selected for system development and repeatable hardware testing. They are **engineering heuristics**, not medical or occupational-health standards.

| Status | Current Logic |
|---|---|
| **GOOD** | Temperature 18–30 °C, humidity 30–70%, light ≥ 300 lx |
| **WARNING** | Valid measurements outside the GOOD range but not severe enough for POOR |
| **POOR** | Temperature < 15 °C or > 32 °C, humidity < 20% or > 80%, or light < 50 lx |

Pressure is measured and displayed but is not currently used in the study-status classification because it provides less immediately actionable indoor feedback than temperature, humidity, and lighting.

If a required environmental sensor is unavailable, the system falls back to `WARNING` instead of reporting a false `GOOD` state.

## Physical Outputs

The environment status directly controls the three LEDs:

```text
GOOD    -> Green LED
WARNING -> Yellow LED
POOR    -> Red LED
```

The buzzer uses **state-transition-based alerting**.

When the environment changes from GOOD or WARNING into POOR, the buzzer produces one short approximately 200 ms alert.

It does not continuously sound while the environment remains POOR.

Once conditions recover and the system later enters POOR again, the alert is re-armed and triggers once again.

This behavior avoids repetitive nuisance alarms while still providing immediate feedback when conditions deteriorate.

## ESP32 Pin Map

| Function | ESP32-S3 GPIO |
|---|---:|
| Green LED | GPIO 4 |
| Yellow LED | GPIO 5 |
| Red LED | GPIO 6 |
| Buzzer | GPIO 7 |
| I²C SDA | GPIO 8 |
| I²C SCL | GPIO 9 |

Each LED is connected through a 220 Ω current-limiting resistor.

## Real Hardware Validation

The system has been tested on the physical breadboard rather than only compiled in software.

| Test | Verified Result |
|---|---|
| Normal room lighting (~120 lx during captured test) | `WARNING`, yellow LED |
| BH1750 completely covered | `POOR`, red LED |
| Enter POOR state | Buzzer sounds once |
| Remain in POOR state | Red LED remains on; buzzer stays silent |
| Recover to WARNING and enter POOR again | Buzzer sounds once again |
| Bright phone flashlight illumination | `GOOD`, green LED when temperature and humidity remain inside GOOD ranges |
| OLED output | Live temperature, humidity, light, pressure, and status displayed |
| Shared I²C bus | BME280, BH1750, and OLED detected and operating together |

The prototype shown above captured approximately:

```text
STATUS: WARNING
Temperature: 27.1 C
Humidity:    46.6 %
Light:       120.8 lx
Pressure:    991.8 hPa
```

## Firmware Design

The firmware separates several responsibilities into dedicated functions:

| Function | Responsibility |
|---|---|
| `evaluateEnvironment()` | Converts sensor measurements into GOOD / WARNING / POOR |
| `updateStatusLeds()` | Updates the green, yellow, and red LEDs |
| `updateBuzzer()` | Detects transitions into POOR and generates the one-shot alert |
| `statusToString()` | Provides a common text representation for Serial and OLED output |

This separation makes the firmware easier to test, explain, and extend than placing all logic directly inside `loop()`.

## Engineering Decisions

### Shared I²C Bus

The BME280, BH1750, and SSD1306 OLED share one SDA/SCL pair.

This reduces GPIO usage and demonstrates integration of multiple independent devices on the same communication bus.

### Transition-Based Alerting

A continuous alarm would create unnecessary noise whenever a POOR condition persisted.

The firmware therefore stores the previous environment state and triggers the buzzer only when the system transitions into POOR.

### Fail-Safe Classification

If required environmental sensor data is unavailable, the firmware does not classify the environment as GOOD.

### Actionable Measurements

Temperature, humidity, and ambient light are currently used for study-condition classification.

Atmospheric pressure remains available as an additional measured value but does not affect the status.

## Build

This project uses PlatformIO with the Arduino framework.

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

Dependencies are defined in `platformio.ini`:

```text
Adafruit BME280 Library
BH1750
Adafruit SSD1306
```

## Development Evidence

Selected hardware evidence is stored under:

```text
docs/images/
```

The public repository intentionally contains selected, useful engineering evidence rather than every intermediate development screenshot.

Additional raw build photos, debugging evidence, and development history are maintained separately for project documentation and interview preparation.

## AI-Assisted Development

AI-assisted tools were used during development for tasks such as debugging support, code review, and iteration.

Physical assembly, wiring changes, sensor integration, hardware testing, observed-result verification, and engineering acceptance decisions were performed and validated on the real prototype.

The goal was to use AI as an engineering productivity tool while maintaining understanding of the system behavior and validating generated suggestions against physical hardware.

## Current Status

The main hardware feature set is operational:

```text
BME280 sensing        PASS
BH1750 sensing        PASS
Shared I2C bus        PASS
OLED dashboard        PASS
Three-state LEDs      PASS
Transition alert      PASS
Physical validation   PASS
```

The remaining work focuses primarily on firmware robustness, repeatable testing, and recruiter-facing project polish.

## Planned Improvements

| Improvement | Purpose |
|---|---|
| Non-blocking timing with `millis()` | Avoid unnecessary blocking delays |
| Status hysteresis | Prevent rapid state changes near thresholds |
| Stronger sensor fault handling | Improve robustness |
| Repeatable validation cases | Produce clearer engineering evidence |
| Wiring / system documentation | Improve reproducibility |
| Final project photos | Improve presentation quality |
| Recruiter demo material | Make the project easier to evaluate quickly |
