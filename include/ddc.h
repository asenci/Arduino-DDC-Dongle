#ifndef DDC_H
#define DDC_H

#include <Arduino.h>

const uint8_t DDC_EDID_LENGTH = 128;
const uint8_t DDC_EDID_EXTEN_COUNT_POS = 126;

const uint8_t DDC_I2C_PRI_ADDR = 0xA0u >> 1u;
const uint8_t DDC_I2C_SEC_ADDR = 0xA4u >> 1u;
const uint8_t DDC_I2C_SP_ADDR = 0x60u >> 1u;

const uint8_t DDC_I2C_ALL_ADDR = DDC_I2C_PRI_ADDR | DDC_I2C_SEC_ADDR | DDC_I2C_SP_ADDR;
const uint8_t DDC_I2C_ALL_MASK = DDC_I2C_ALL_ADDR ^ (DDC_I2C_PRI_ADDR & DDC_I2C_SEC_ADDR & DDC_I2C_SP_ADDR);

// Split EEPROM between primary and secondary DDC memory
const uint16_t DDC_MEM_LENGTH = (E2END + 1) / 2;


void ddcDumpEdid();

size_t ddcLoadEeprom(uint8_t *buffer, uint16_t offset = 0);

void ddcReceiveCommand(int length);

void ddcReadRequest();

bool ddcRequestEdid(uint8_t *data, uint8_t dataSize, uint16_t offset = 0, uint8_t i2cAddress = DDC_I2C_PRI_ADDR);

bool ddcSetOffset(uint8_t address = DDC_I2C_PRI_ADDR, uint8_t offset = 0);

void ddcSetup();

bool ddcVerifyChecksum(uint8_t *data, uint8_t dataSize);

#endif //DDC_H
