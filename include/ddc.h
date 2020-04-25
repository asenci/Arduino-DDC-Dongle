#ifndef DDC_H
#define DDC_H

#include <Arduino.h>

const int edidUnitSize = 128;
const int edidExtCountPos = 126;

const uint8_t ddcPriAddress = 0xA0u >> 1u;
const uint8_t ddcSecAddress = 0xA4u >> 1u;
const uint8_t ddcSpAddress = 0x60u >> 1u;

const uint8_t ddcAllAddresses = ddcPriAddress | ddcSpAddress;
const uint8_t ddcAllAddressesMask = ddcAllAddresses ^(ddcPriAddress & ddcSpAddress);

void dumpEDID();

void receiveDdcCommand(int length);

void receiveDdcReadRequest();

bool readDdcData(uint8_t *buffer, uint8_t address);

bool requestEdidAtOffset(uint8_t *data, int dataSize, int offset = 0, int i2cAddress = ddcPriAddress);

bool setDdcOffset(uint8_t address = ddcPriAddress, uint8_t offset = 0);

bool verifyEdidCheckSum(uint8_t *data, int dataSize);

#endif //DDC_H
