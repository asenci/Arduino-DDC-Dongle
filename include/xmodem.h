#ifndef XMODEM_H
#define XMODEM_H

#include <Arduino.h>

#ifndef XMODEM_DEFAULT_NAK_INTERVAL
#define XMODEM_DEFAULT_NAK_INTERVAL 10000
#endif //XMODEM_DEFAULT_NAK_INTERVAL

const int xModemBlockPayloadSize = 128;

bool xModemDownload();

void xModemMainLoop(unsigned long nakInternal = XMODEM_DEFAULT_NAK_INTERVAL);

int xModemReadBlock(uint8_t *buffer, uint8_t expectedBlockSeq = 1);

bool xModemReadByte(uint8_t *buffer, size_t length = 1, unsigned long timeout = 1000, int tries = 1, bool nak = false);

void xModemSendBlock(uint8_t *buffer, uint8_t blockSeq = 1);

bool xModemUpload();

#endif //XMODEM_H
