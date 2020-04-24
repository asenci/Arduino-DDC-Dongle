#ifndef XMODEM_H
#define XMODEM_H

#include <Arduino.h>

#ifndef NODEBUG
#define XMODEM_DEFAULT_NAK_INTERVAL 1000
#else //NODEBUG
#define XMODEM_DEFAULT_NAK_INTERVAL 10000
#endif //NODEBUG

const int xModemBlockSize = 132;
const int xModemBlockCmdPos = 0;
const int xModemBlockSeqPos = 1;
const int xModemBlockRevSeqPos = 2;
const int xModemBlockPayloadPos = 3;
const int xModemBlockPayloadSize = 128;
const int xModemBlockChecksumPos = xModemBlockSize - 1;

bool xModemDownload();

void xModemFlush();

void xModemMainLoop(long nakInternal = XMODEM_DEFAULT_NAK_INTERVAL);

bool xModemReadBlock(byte *data);

bool xModemReadCmd(byte *data, unsigned long timeout = 10000, int tries = 1, bool nak = false);

bool xModemUpload();

#endif //XMODEM_H
