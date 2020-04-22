#ifndef DDC
#define DDC

const int ddcDataUnitSize = 128;

const int ddcPriAddress = 0xA0 >> 1;
const int ddcSecAddress = 0xA4 >> 1;
const int ddcSpAddress = 0x60 >> 1;

void receiveDdcCommand(int numBytes);
void sendDdcData();

#endif //DDC
