/*
MCU Pin Assignment
*/
const int dataPin   = 7;
const int clockPin  = 6;
const int selectPin = 5;

// Reserved GPIOs (do not use for peripherals)
const int reservedGpio0 = 0;
const int reservedGpio3 = 3;
const int reservedGpio4 = 4;

// I2C defaults for ESP32-C3-DevKitM-1 (Arduino core)
const int i2cSdaPin = 8;
const int i2cSclPin = 9;


//TODO: Rename command set from RUN to TRIGGER
//TaskHandle_t serialCommandMonitor_t;    // Command and Control
//SPIClass *fSPI = new SPIClass(SPI);     // ESP32‑C3 uses SPI/FSPI, HSPI is not valid

//Prototypes
void fuel_Motor_Control();
void fuelMotorEnablePIN(bool value);
void serialCommandMonitor();                // -- Serial Command Handler

//STRING SHIZ
#define xstr(s) str(s)
#define str(s) #s
#define SHOWSYM(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is %ld\n"), (long)a); Serial.print(spbuffer); } while (0)
#define SHOWSYMS(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is '" a "'\n")); Serial.print(spbuffer); } while (0)
#define mySerial Serial

//uint32_t start, stop; //temp code timing -- shit code include... TODO: REFACTOR
//float temp = 0;
