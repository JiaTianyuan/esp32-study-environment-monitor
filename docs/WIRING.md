# Hardware Wiring

This document records the hardware connections used by the ESP32 Study Environment Monitor prototype.

The logical GPIO mapping is the authoritative wiring reference. Breadboard coordinates are included separately as a record of the physical prototype used during validation.

## System Wiring Overview

```text
                         ESP32-S3
                            |
             +--------------+--------------+
             |              |              |
          GPIO8 SDA       GPIO9 SCL       3.3V / GND
             |              |              |
      +------+------+       |       +------+------+
      |      |      |       |       |      |      |
   BME280  BH1750  OLED     +---- BME280 BH1750 OLED
      |      |      |
    0x76   0x23    0x3C

GPIO4  ---> Green LED
GPIO5  ---> Yellow LED
GPIO6  ---> Red LED
GPIO7  ---> Active buzzer I/O
```

## ESP32 Pin Map

| ESP32 Pin | Function | Connected Device |
|---|---|---|
| `3V3` | 3.3 V power | BME280, BH1750, OLED, buzzer |
| `GND` | Common ground | All modules and LED ground rail |
| `GPIO8` | I²C SDA | BME280, BH1750, OLED |
| `GPIO9` | I²C SCL | BME280, BH1750, OLED |
| `GPIO4` | Digital output | Green LED |
| `GPIO5` | Digital output | Yellow LED |
| `GPIO6` | Digital output | Red LED |
| `GPIO7` | Digital output | Active buzzer signal |

## Shared I²C Bus

The BME280, BH1750, and SSD1306 OLED share the same SDA and SCL lines.

```text
ESP32 GPIO8 (SDA)
    |
    +---- BME280 SDA
    +---- BH1750 SDA
    +---- OLED SDA

ESP32 GPIO9 (SCL)
    |
    +---- BME280 SCL
    +---- BH1750 SCL
    +---- OLED SCL
```

All three devices were detected and operated simultaneously during hardware validation.

| Device | I²C Address | Connection |
|---|---:|---|
| BH1750 | `0x23` | ADDR tied to GND |
| SSD1306 OLED | `0x3C` | Shared I²C bus |
| BME280 | `0x76` | Shared I²C bus |

The firmware also attempts `0x77` as a fallback BME280 address.

## BME280

The BME280 provides temperature, humidity, and atmospheric pressure measurements.

| BME280 Pin | Connection |
|---|---|
| `VCC` | ESP32 `3V3` |
| `GND` | ESP32 `GND` |
| `SCL` | ESP32 `GPIO9` |
| `SDA` | ESP32 `GPIO8` |

Prototype breadboard placement:

```text
A30 = VCC
A31 = GND
A32 = SCL
A33 = SDA
```

## BH1750 / GY-302

The BH1750 measures ambient illuminance in lux.

| BH1750 Pin | Connection |
|---|---|
| `VCC` | 3.3 V power rail |
| `GND` | Common ground |
| `SCL` | ESP32 `GPIO9` |
| `SDA` | ESP32 `GPIO8` |
| `ADDR` | GND |

Tying `ADDR` to ground configures the module at I²C address `0x23`.

Prototype breadboard placement:

```text
A40 = ADDR
A41 = SDA
A42 = SCL
A43 = GND
A44 = VCC
```

## SSD1306 OLED

The 128×64 OLED displays live sensor measurements and the current environment state.

| OLED Pin | Connection |
|---|---|
| `SDA` | ESP32 `GPIO8` |
| `SCL` | ESP32 `GPIO9` |
| `VCC` | 3.3 V power rail |
| `GND` | Common ground |

Prototype breadboard placement:

```text
A50 = SDA
A51 = SCL
A52 = VCC
A53 = GND
```

The OLED operates at I²C address `0x3C`.

## Status LEDs

Three LEDs provide an immediate visual indication of the classified environment state.

| State | LED | ESP32 Pin |
|---|---|---|
| GOOD | Green | `GPIO4` |
| WARNING | Yellow | `GPIO5` |
| POOR | Red | `GPIO6` |

Each LED is connected through a current-limiting resistor before returning to the common ground rail.

### Green LED

```text
GPIO4 ---> LED anode (+)
LED cathode (-) ---> resistor ---> GND
```

Prototype placement:

```text
J30 = short leg / cathode (-)
J31 = long leg / anode (+)

GPIO4 ---> I31
I30 ---> resistor ---> lower GND rail
```

### Yellow LED

```text
GPIO5 ---> LED anode (+)
LED cathode (-) ---> resistor ---> GND
```

Prototype placement:

```text
J34 = short leg / cathode (-)
J35 = long leg / anode (+)

GPIO5 ---> I35
I34 ---> resistor ---> lower GND rail
```

### Red LED

```text
GPIO6 ---> LED anode (+)
LED cathode (-) ---> resistor ---> GND
```

Prototype placement:

```text
J38 = short leg / cathode (-)
J39 = long leg / anode (+)

GPIO6 ---> I39
I38 ---> resistor ---> lower GND rail
```

## Active Buzzer

The active buzzer module provides a short audible alert when the system transitions into the POOR state.

Module pin order used by the prototype:

```text
GND | I/O | VCC
```

Connections:

| Buzzer Pin | Connection |
|---|---|
| `GND` | Common ground |
| `I/O` | ESP32 `GPIO7` |
| `VCC` | 3.3 V power |

Prototype breadboard placement:

```text
J44 = GND
J45 = I/O
J46 = VCC

GPIO7 ---> I45
I44 ---> GND rail
I46 ---> 3.3 V rail
```

The firmware controls the buzzer with non-blocking timing and generates approximately one 200 ms pulse when entering POOR.

Remaining in POOR does not continuously retrigger the buzzer.

## Power Distribution

The prototype uses a common 3.3 V supply and common ground across the sensor modules, OLED, buzzer, and LED circuitry.

```text
ESP32 3V3
    |
    +---- BME280 VCC
    +---- BH1750 VCC
    +---- OLED VCC
    +---- Buzzer VCC

ESP32 GND
    |
    +---- BME280 GND
    +---- BH1750 GND
    +---- OLED GND
    +---- Buzzer GND
    +---- LED ground rail
```

Using a common ground is required so that all digital and I²C signals share the same voltage reference.

## Validated Hardware Configuration

The physical prototype has been tested with:

- BME280 temperature, humidity, and pressure sensing
- BH1750 ambient-light sensing
- Three devices operating simultaneously on one I²C bus
- SSD1306 live OLED output
- Green, Yellow, and Red status LEDs
- Transition-based active buzzer alert
- Runtime sensor-value validation
- Sensor-error fail-safe behavior
- Stateful environment classification with hysteresis

Detailed physical test evidence is recorded in [`TESTING.md`](TESTING.md).

## Reproducibility Note

The logical GPIO and module-pin mappings in this document should be used when reproducing the project.

The breadboard coordinates describe the specific physical prototype used during development and testing. They are included as implementation evidence rather than as a requirement for recreating the circuit on a different breadboard layout.
