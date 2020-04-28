#ifndef DEBUG_H
#define DEBUG_H

//#define NODEBUG

#ifndef NODEBUG

#include <Arduino.h>

#ifdef ARDUINO_AVR_LEONARDO
#define SerialDebug Serial1
#define SetupSerialDebug SerialDebug.begin(115200);
#else
#define SerialDebug Serial
#define SetupSerialDebug
#endif

void dumpByte(uint8_t b);

void hexDump(const __FlashStringHelper *name, uint8_t *data, uint16_t length = 1, uint8_t indent = 0);

void hexDump(uint8_t *data, uint16_t length = 1, uint8_t indent = 0);


#else //NODEBUG
#define dumpByte(...)
#define hexDump(...)
#define SerialDebug if(0)Serial
#define SetupSerialDebug

#endif //NODEBUG

#endif //DEBUG_H
