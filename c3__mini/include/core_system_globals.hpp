#ifndef CORE_SYSTEM_GLOBALS_H
#define CORE_SYSTEM_GLOBALS_H


#define xstr(s) str(s)
#define str(s) #s
#define SHOWSYM(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is %ld\n"), (long)a); Serial.print(spbuffer); } while (0)
#define SHOWSYMS(a) do {sprintf(spbuffer, PSTR("Value of '" #a "' is '" a "'\n")); Serial.print(spbuffer); } while (0)
#define mySerial Serial

//Prototypes
void serialCommandMonitor();                // -- Serial Command Handler
float readVoltageAveraged(int);



#endif // CORE_SYSTEM_GLOBALS_H