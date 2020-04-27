#ifndef DEBUG_H
#define DEBUG_H

//#define NODEBUG

#ifndef NODEBUG

#include <Arduino.h>

#define SerialDebug Serial1

void dumpByte(uint8_t b);

void hexDump(const __FlashStringHelper *name, uint8_t *data, uint16_t length = 1, uint8_t indent = 0);

void hexDump(uint8_t *data, uint16_t length = 1, uint8_t indent = 0);


#else //NODEBUG
#define dumpByte(...)
#define hexDump(...)
#define SerialDebug if(0)Serial1

#endif //NODEBUG

#endif //DEBUG_H
