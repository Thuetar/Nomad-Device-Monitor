# Nomad Device Monitor

** Sourced From: Base Device Template **

## Targetting ESP32 C3 Super-Mini Platform
## IO
### System Integrated IO Devices
- AHT10 -- Environment Temp & RH, I2C 
- WCS1800 -- Current sensor, GPIO 0 (Analog/AOUT) 
-- TODO: Requires a lot of calibrations prior to use... Calibration is REQ!

NEXT: Web Server & REST or BLE?

_________

## I2C, SPI & More
*I2C*
SDA GPIO 8 
SCL GPIO 9
(SPI == Status Display)

## Future Pin Assignments
GPIO 01 // UI
GPIO 02 // UI
GPIO 03

# Name Spaces
https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
Name Spaces
namespace overseer::device::energy
