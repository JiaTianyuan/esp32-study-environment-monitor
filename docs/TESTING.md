# Hardware Validation

This document records repeatable validation performed on the physical ESP32 Study Environment Monitor prototype.

The goal is to verify not only that the firmware compiles, but that the sensors, state logic, hysteresis, outputs, and fail-safe behavior operate correctly on real hardware.

## Test Platform

- Controller: ESP32-S3
- Environmental sensor: BME280
- Light sensor: BH1750 / GY-302
- Display: 128×64 SSD1306 OLED
- Outputs: Green, Yellow, and Red LEDs
- Alert: Active buzzer module
- Firmware: Arduino C++
- Build system: PlatformIO
- Serial baud rate: 115200

## I2C Integration

The following devices were detected and operated simultaneously on the shared I2C bus:

| Device | Address | Result |
|---|---:|---|
| BH1750 | `0x23` | PASS |
| SSD1306 OLED | `0x3C` | PASS |
| BME280 | `0x76` | PASS |

The firmware also supports `0x77` as a fallback BME280 address.

## Test 1 — Baseline Sensor Operation

### Objective

Verify that the BME280 and BH1750 provide stable real-world measurements and that the OLED and Serial Monitor display live data.

### Representative Observation

```text
Temperature: 27.00 C
Humidity: 50.41 %
Pressure: 994.59 hPa
Light: 25.00 lx
Sensor validation: PASS
Status: POOR
```

### Result

PASS

The sensor values were successfully read and validated by the firmware.

The `POOR` result was expected because the measured light level was below the configured 50 lx POOR-entry threshold.

## Test 2 — WARNING State

### Objective

Verify the intermediate environment state and yellow LED output.

### Procedure

Expose the BH1750 to normal indoor lighting while temperature and humidity remain within acceptable ranges.

### Representative Observations

Normal indoor-light measurements during testing were approximately 120–170 lx.

### Expected Result

```text
Sensor validation: PASS
Status: WARNING
```

Yellow LED active.

### Result

PASS

## Test 3 — POOR State

### Objective

Verify detection of a severe low-light condition.

### Procedure

Cover the BH1750 until the measured light level falls below 50 lx.

### Expected Result

```text
Status: POOR
```

Red LED active.

### Result

PASS

A representative low-light measurement was:

```text
Light: 25.00 lx
Sensor validation: PASS
Status: POOR
```

## Test 4 — Transition-Based Buzzer Alert

### Objective

Verify that the buzzer alerts only when the environment enters the POOR state.

### Procedure

1. Begin in WARNING.
2. Reduce light below the POOR threshold.
3. Keep the system in POOR.
4. Recover to WARNING.
5. Enter POOR again.

### Expected Behavior

- First transition into POOR: one short buzzer alert.
- Remaining in POOR: no repeated buzzer alerts.
- Recovery from POOR: no alert.
- Entering POOR again: one new short alert.

### Result

PASS

The buzzer successfully re-armed after recovery and did not continuously sound while the system remained POOR.

## Test 5 — POOR-State Light Hysteresis

### Objective

Verify that small light changes around the POOR boundary do not cause rapid state switching.

### Procedure

1. Reduce light below 50 lx to enter POOR.
2. Slowly increase light into the 50–70 lx range.
3. Increase light beyond 70 lx.

### Expected Behavior

| Light Condition | Expected State |
|---|---|
| `< 50 lx` | Enter POOR |
| `50–70 lx` after entering POOR | Remain POOR |
| `≥ 70 lx` with other recovery conditions satisfied | Recover from POOR |

### Result

PASS

The system remained POOR between approximately 50 and 70 lx and recovered only after the configured recovery boundary was reached.

## Test 6 — GOOD-State Light Hysteresis

### Objective

Verify that the GOOD state remains stable near its light threshold.

### Procedure

1. Increase light to at least 330 lx while temperature and humidity satisfy the GOOD entry ranges.
2. Reduce light into the 300–330 lx range.
3. Reduce light below 300 lx.

### Expected Behavior

| Light Condition | Expected State |
|---|---|
| `≥ 330 lx` with other GOOD conditions satisfied | Enter GOOD |
| `300–330 lx` after entering GOOD | Remain GOOD |
| `< 300 lx` | Leave GOOD |

### Result

PASS

The system entered GOOD above the stricter entry threshold, remained GOOD inside the hysteresis band, and returned to WARNING below the hold threshold.

## Test 7 — POOR-to-WARNING Recovery

### Objective

Verify that the final firmware preserves the intended hysteresis behavior after runtime sensor validation was added.

### Initial Observation

```text
Temperature: 27.00 C
Humidity: 50.41 %
Pressure: 994.59 hPa
Light: 25.00 lx
Sensor validation: PASS
Status: POOR
```

### Recovery Observation

After increasing illumination:

```text
Temperature: 27.01 C
Humidity: 51.36 %
Pressure: 994.59 hPa
Light: 160.83 lx
Sensor validation: PASS
Status: WARNING
```

### Result

PASS

The system recovered from POOR to WARNING after the light level exceeded the configured POOR recovery threshold.

## Test 8 — Runtime Sensor Validation

### Objective

Verify that valid sensor readings are accepted before environment classification.

### Representative Observation

```text
Temperature: 26.00 C
Humidity: 61.52 %
Pressure: 994.67 hPa
Light: 135.83 lx
Sensor validation: PASS
Status: WARNING
```

### Result

PASS

Valid finite sensor values inside the configured sensor operating ranges were accepted.

## Test 9 — Sensor Fault Injection

### Objective

Verify fail-safe behavior when a sensor produces invalid runtime data.

### Method

A temporary software fault was introduced during testing:

```cpp
lux = NAN;
```

This line was used only for validation and was removed before the final firmware was committed.

### Expected Behavior

```text
Sensor validation: FAILED
Fail-safe: forcing WARNING state.
Status: WARNING (SENSOR ERROR)
```

The OLED should report:

```text
STATUS: SENSOR ERROR
```

and the system should avoid reporting a false GOOD state.

### Result

PASS

The firmware detected the injected invalid reading and forced the system into the fail-safe WARNING state.

After the temporary fault injection was removed, normal sensor readings again produced:

```text
Sensor validation: PASS
```

## Test 10 — Non-Blocking Runtime Behavior

### Objective

Verify that periodic sensor updates and the buzzer alert operate without the previous blocking runtime delays.

### Implementation

- Sensor sampling interval: 2000 ms using `millis()`
- Buzzer pulse: approximately 200 ms using timestamp-based control

### Result

PASS

The firmware continued periodic sensor updates while retaining transition-based buzzer behavior.

## Validation Summary

| Validation Area | Result |
|---|---|
| BME280 sensing | PASS |
| BH1750 sensing | PASS |
| Shared I2C bus | PASS |
| OLED output | PASS |
| Three-state LED output | PASS |
| POOR detection | PASS |
| WARNING detection | PASS |
| GOOD detection | PASS |
| Transition-based buzzer | PASS |
| POOR-state light hysteresis | PASS |
| GOOD-state light hysteresis | PASS |
| Runtime sensor validation | PASS |
| Sensor-error fail-safe | PASS |
| Recovery after injected fault | PASS |
| Non-blocking runtime behavior | PASS |

## Current Validation Scope

Light-threshold hysteresis has been exercised directly on the physical prototype.

Temperature and humidity hysteresis are implemented in firmware but have not yet been physically driven through every entry, hold, and recovery boundary.

This distinction is intentionally preserved so that the documented test evidence reflects only behavior that has actually been validated.
