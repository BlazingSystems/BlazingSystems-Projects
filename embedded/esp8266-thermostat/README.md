# ESP8266 Thermostat

**Type:** Embedded firmware with local web interface  
**Status:** Source-ready; hardware validation required

An offline thermostat controller for ESP8266 boards using a DS18B20 temperature sensor and relay output.

## Main Functions

- ESP8266 local access point;
- browser-based control/status interface;
- automatic setpoint and hysteresis control;
- minimum relay on/off timing;
- manual override;
- calibration offset;
- EEPROM-persistent settings;
- per-device generated access-point password.

## Hardware

Typical wiring in the source:

- DS18B20 data: GPIO4 / D2;
- relay input: GPIO5 / D1;
- 4.7 kΩ DS18B20 pull-up to 3.3 V.

## Preview

Open `preview.html` for a browser-only representation of the device UI.

## Validation

The source is suitable for bench testing but should not be treated as a certified mains controller. Verify sensor accuracy, relay polarity, fail-safe state, anti-short-cycle behavior, enclosure, isolation, and electrical ratings on the intended hardware.
