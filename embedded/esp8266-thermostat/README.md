# ESP8266 Thermostat

**Status:** SOURCE READY / HARDWARE VALIDATION REQUIRED  
**Canonical recovered artifact:** `ESP8266_Thermostat.ino`

Standalone ESP8266 thermostat firmware with its own Wi-Fi AP and local web interface.

## Recovered features

- Local AP mode; no cloud dependency
- Main / Settings / Calibration / System web tabs
- DS18B20 temperature probe support
- Relay control
- Automatic thermostat mode with hysteresis
- Minimum relay ON/OFF delay / anti-short-cycle protection
- Manual relay mode
- Temperature calibration offset
- Persistent EEPROM settings

## Validation status

The source is complete and internally consistent as recovered. It is published as **source-ready**, not as a guaranteed production binary, because final behavior still depends on the exact ESP8266 board, relay wiring, sensor wiring, power supply, and installed Arduino libraries.

## Build notes

Compile for the intended ESP8266 board using the Arduino ESP8266 core and the libraries referenced at the top of the sketch. Verify the configured GPIO pins before powering a relay or load.

## Safety

Do not connect mains voltage directly to the ESP8266. Use appropriately rated, isolated relay hardware and safe enclosure/wiring practices.

## Provenance

Recovered from the BlazingSystems ChatGPT project archive and reconciled against the available project history before publication.

## Recovery repair and hardening

During the migration audit, the first GitHub copy was found to be truncated at the start of `setup()`. It was replaced from the complete recovered Library artifact; the maintained source now contains the full `setup()` and `loop()` implementation.

The untouched complete recovered source is preserved in BlazingSystems-Archives. The maintained Projects copy replaces the universal `thermostat` AP password with a per-device `BlazeTherm-<chip-id>` default. Change that default for real deployments.

A target ESP8266 compile/hardware test is still required; this environment does not have Arduino CLI/PlatformIO installed.
