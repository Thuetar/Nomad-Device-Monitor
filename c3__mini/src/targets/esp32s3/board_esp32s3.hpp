#ifndef BOARD_ESP32S3_HPP
#define BOARD_ESP32S3_HPP

/*
ESP32-S3-DevKitM-1
-- GPIO26-GPIO32 are usually used for the ESP32-S3’s flash/PSRAM and are not recommended for general use.
-- On some octal-flash/PSRAM variants, GPIO33-GPIO37 are also reserved. ???
*/
const int AMP_SENSE_PIN = 15; // WCS1800
const int BLUE_STATUS_LED = 16; // Blue Led
constexpr uint8_t TOUCH_PINS[] = {5};
constexpr uint8_t LED_PIN = 38;
constexpr unsigned long READ_INTERVAL_MS = 250;
constexpr size_t BASELINE_SAMPLES = 16;

// SPI Pin Assignment
const int selectPin = 5;
const int clockPin  = 6;
const int dataPin   = 7;

// I2C Pins 
static const int i2cSdaPin = 8;
static const int i2cSclPin = 9;
static const int WATER_LEVEL_SENSE_PIN = 5;


//static const int ADC_PIN = 0;          // <- set to your ADC GPIO (Future maybe)
static const float VCC   = 3.3f;       // board supply voltage (3.3 if powered from 3V3)
static const int   ADC_MAX = 4095;     // ESP32 ADC is 12-bit in Arduino core

// Cal values (start with these, then replace after calibration)
static float v0 = 1.65f;               // zero-current midpoint (measure it!)
static float mv_per_a = 66.0f;         // placeholder; you MUST calibrate your board




#endif // BOARD_ESP32S3_HPP
