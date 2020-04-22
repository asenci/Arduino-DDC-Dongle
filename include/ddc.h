#ifndef DDC_H
#define DDC_H

const int edidUnitSize = 128;
const int edidNumExtPos = 126;

const int ddcPriAddress = 0xA0 >> 1;
const int ddcSecAddress = 0xA4 >> 1;
const int ddcSpAddress = 0x60 >> 1;

void receiveDdcCommand(int numBytes);
void sendDdcData();

void dumpEDID();
bool readAtOffset(byte *data, int dataSize);
bool readAtOffset(byte *data, int dataSize, int offset);
bool readAtOffset(byte *data, int dataSize, int offset, int i2cAddress);
bool hexDump(byte *data, int dataSize);
bool verifyCheckSum(byte *data, int dataSize);

#endif //DDC_H
