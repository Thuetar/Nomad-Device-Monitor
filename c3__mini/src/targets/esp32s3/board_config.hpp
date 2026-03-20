#ifndef BOARD_CONFIG
#define BOARD_CONFIG

/*
ESP32-C3-DevKitM-1
*/
static const int AMP_SENSE_PIN = 0; // WCS1800

// Reserved GPIOs --> for play & Future IO 
//const int reservedGpio0 = 0; 
//const int reservedGpio3 = 3; // Blue Led
const int BLUE_STATUS_LED = 3; // Blue Led
//const int reservedGpio4 = 4; // Green


// SPI Pin Assignment
const int selectPin = 5;
const int clockPin  = 6;
const int dataPin   = 7;

// I2C Pins 
static const int i2cSdaPin = 8;
static const int i2cSclPin = 9;
static const int WATER_LEVEL_SENSE_PIN = 10;


//static const int ADC_PIN = 0;          // <- set to your ADC GPIO (Future maybe)
static const float VCC   = 3.3f;       // board supply voltage (3.3 if powered from 3V3)
static const int   ADC_MAX = 4095;     // ESP32 ADC is 12-bit in Arduino core

// Cal values (start with these, then replace after calibration)
static float v0 = 1.65f;               // zero-current midpoint (measure it!)
static float mv_per_a = 66.0f;         // placeholder; you MUST calibrate your board




#endif //BOARD_CONFIG

