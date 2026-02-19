
/*
ESP32-C3-DevKitM-1
*/
// I2C Pins 
static const int i2cSdaPin = 8;
static const int i2cSclPin = 9;

static const int AMP_SENSE_PIN = 0; // WCS1800
//static const int ADC_PIN = 0;          // <- set to your ADC GPIO
static const float VCC   = 3.3f;       // board supply voltage (3.3 if powered from 3V3)
static const int   ADC_MAX = 4095;     // ESP32 ADC is 12-bit in Arduino core

// Cal values (start with these, then replace after calibration)
static float v0 = 1.65f;               // zero-current midpoint (measure it!)
static float mv_per_a = 66.0f;         // placeholder; you MUST calibrate your board

/*
// SPI Pin Assignment
*/
const int dataPin   = 7;
const int clockPin  = 6;
const int selectPin = 5;

// Reserved GPIOs --> for play & Future IO 
//const int reservedGpio0 = 0; // Blue PWM Led
//const int reservedGpio3 = 3; // Blue Led
//const int reservedGpio4 = 4; // Green



//Prototypes
//void fuel_Motor_Control();
//void fuelMotorEnablePIN(bool value);
void serialCommandMonitor();                // -- Serial Command Handler
float readVoltageAveraged(int);
//STRING SHIZ
#define xstr(s) str(s)
#define str(s) #s
#define SHOWSYM(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is %ld\n"), (long)a); Serial.print(spbuffer); } while (0)
#define SHOWSYMS(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is '" a "'\n")); Serial.print(spbuffer); } while (0)
#define mySerial Serial

//uint32_t start, stop; //temp code timing -- shit code include... TODO: REFACTOR
//float temp = 0;
