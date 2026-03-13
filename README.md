# Nomad Device Monitor

## Platform: Currently Targetting ESP32 C3 Super-Mini Platform
### Hardware Stuff 
** GPIO Pin Assignments**
- 0 Amp/Current Sensor (WCS1800)
- 3 BLUE_STATUS_LED
- 5 select (SPI)
- 6 clockPin (SPI)
- 7 dataPin (SPI)
- 8 SDA GPIO 8 * I2C *
- 9 SCL GPIO 9 * I2C *
- 10 Water Level?


## Future Pin Assignments
GPIO 01 // UI
GPIO 02 // UI
GPIO 03 // UI



## Todo and whatall
1. Node/System Config & Preferences 
2. REST & Web Page



## IO 
* Water Level Sensing *

### System Integrated IO Devices
- AHT10 -- Environment Temp & RH, *I2C* 
- WCS1800 -- Current sensor, GPIO 0 (*Analog/AOUT*) 
-- TODO: Requires a lot of calibrations prior to use... Calibration is REQ!

NEXT: Web Server & REST for tank readings? 



# Dev Stuff
comment types: 
- @BUG
- @TODO
- @Warning 
- @Attention
- @Note
- @Remark

## Config/Constants 
See 
# Name Spaces
https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
Name Spaces
namespace overseer::device::energy

** Sourced From: Nomad Base Device Template **


