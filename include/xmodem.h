#ifndef XMODEM
#define XMODEM

#include <Arduino.h>

const int xModemBlockSize = 132;
const int xModemBlockCmdPos = 0;
const int xModemBlockSeqPos = 1;
const int xModemBlockRevSeqPos = 2;
const int xModemBlockPayloadPos = 3;
const int xModemBlockPayloadSize = 128;
const int xModemBlockChecksumPos = xModemBlockSize - 1;


void xModemFlush();

bool xModemReadBlock(byte *data);

bool xModemReadCmd(byte *data);

bool xModemReadCmd(byte *, unsigned long timeout);

bool xModemReadCmd(byte *, unsigned long timeout, int tries);

#endif //XMODEM
