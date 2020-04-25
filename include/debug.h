#ifndef DEBUG_H
#define DEBUG_H

//#define NODEBUG

#ifndef NODEBUG

#include <Arduino.h>
#include <SoftwareSerial.h>

void dumpByte(uint8_t b);

void hexDump(const __FlashStringHelper *name, uint8_t *data, int length = 1, int indent = 0);

void hexDump(uint8_t *data, int length = 1, int indent = 0);

extern SoftwareSerial SerialDebug;

#else //NODEBUG
#define dumpByte(x)
#define hexDump(x, y, z)
#define SerialDebug if(0)Serial

#endif //NODEBUG

#endif //DEBUG_H