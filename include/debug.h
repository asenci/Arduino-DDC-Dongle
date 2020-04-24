#ifndef DEBUG_H
#define DEBUG_H

//#define NODEBUG

#ifndef NODEBUG
#include <SoftwareSerial.h>

bool hexDump(byte *data, int dataSize);
extern SoftwareSerial SerialDebug;

#else //NODEBUG
#define SerialDebug if(0)Serial
#define hexDump(x, y)

#endif //NODEBUG

#endif //DEBUG_H
