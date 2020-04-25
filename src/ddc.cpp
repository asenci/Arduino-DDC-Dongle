#include <ddc.h>
#include <debug.h>
#include <EEPROM.h>
#include <Wire.h>


// Variables
volatile uint8_t ddcSegmentPointer = 0;
volatile uint8_t ddcPriWordOffset = 0;
volatile uint8_t ddcSecWordOffset = 0;


void dumpEDID() {
    uint8_t rxBuffer[edidUnitSize];

    SerialDebug.println(F("*** Reading sink primary EDID data ***"));

    // Read primary EDID data
    if (!requestEdidAtOffset(rxBuffer, edidUnitSize)) {
        SerialDebug.println(F("*** Error reading EDID data ***"));
        return;
    }

    hexDump(rxBuffer, edidUnitSize);

    if (!verifyEdidCheckSum(rxBuffer, edidUnitSize)) {
        SerialDebug.println(F("*** Invalid checksum for sink primary EDID data ***"));
        return;
    }

    int numExtensions = rxBuffer[edidExtCountPos];
    SerialDebug.print(F("*** EDID has "));
    SerialDebug.print(numExtensions, DEC);
    SerialDebug.println(F(" extensions ***"));

    for (int i = 1; i <= numExtensions; i++) {
        SerialDebug.print(F("*** Reading EDID extension "));
        SerialDebug.print(i, DEC);
        SerialDebug.println(F(" ***"));

        int offset = i * edidUnitSize;
        if (!requestEdidAtOffset(rxBuffer, edidUnitSize, offset)) {
            SerialDebug.println(F("*** Error reading EDID extension "));
            SerialDebug.print(i, DEC);
            SerialDebug.println(F(" ***"));
            continue;
        }

        hexDump(rxBuffer, edidUnitSize);

        if (!verifyEdidCheckSum(&rxBuffer[offset], edidUnitSize)) {
            SerialDebug.print(F("*** Invalid checksum for EDID extension "));
            SerialDebug.print(i, DEC);
            SerialDebug.println(F(" ***"));
            continue;
        }
    }
}

bool readDdcData(uint8_t *buffer, uint8_t address) {
    int eepromOffset = 0;

    switch (address >> 1u) {
        case ddcPriAddress:
            SerialDebug.print(F("Word offset: 0x"));
            SerialDebug.println(ddcPriWordOffset, HEX);
            eepromOffset = (2 * edidUnitSize * ddcSegmentPointer) + ddcPriWordOffset;
            break;
        case ddcSecAddress:
            SerialDebug.print(F("Word offset: 0x"));
            SerialDebug.println(ddcSecWordOffset, HEX);
            eepromOffset = 0x200 + (2 * edidUnitSize * ddcSegmentPointer) + ddcSecWordOffset;
            break;
        default:
            SerialDebug.println(F("** Invalid DDC address **"));
            return false;
    }

    SerialDebug.print(F("Segment pointer: 0x"));
    SerialDebug.println(ddcSegmentPointer, HEX);

    SerialDebug.print(F("EEPROM offset: 0x"));
    SerialDebug.println(eepromOffset, HEX);

    for (int i = 0; i < edidUnitSize; i++) {
        buffer[i] = EEPROM.read(eepromOffset + i);
    }

    hexDump(buffer, edidUnitSize);
    return true;
}

bool requestEdidAtOffset(uint8_t *data, int dataSize, int offset, int i2cAddress) {
    SerialDebug.print(F("*** Reading "));
    SerialDebug.print(dataSize, DEC);
    SerialDebug.print(F(" bytes from address 0x"));
    SerialDebug.print(i2cAddress << 1, HEX);
    SerialDebug.print(F(" at offset 0x"));
    SerialDebug.print(offset, HEX);
    SerialDebug.println(F(" ***"));

    // Set word offset
    Wire.beginTransmission(i2cAddress);
    Wire.write(offset);
    Wire.endTransmission();

    // Read EDID data
    for (int i = 0; i < dataSize; i++) {
        if (i % edidUnitSize == 0) {
            Wire.requestFrom(i2cAddress, edidUnitSize);
        }

        if (Wire.available() < 1) {
            SerialDebug.println(F("*ERROR: NO DATA AVAILABLE*"));
            return false;
        }

        data[i] = Wire.read();
    }

    // Empty the buffer
    if (Wire.available() > 0) {
        SerialDebug.println(F("*ERROR: BUFFER OVERFLOW*"));
    }

    return true;
}

void receiveDdcCommand(int length) {
    uint8_t address = TWDR;
    SerialDebug.print(F("**** Processing DDC command on address 0x"));
    SerialDebug.print(address, HEX);
    SerialDebug.println(F(" ****"));

    uint8_t cmd[length];
    for (int i = 0; i < length; i++) {
        cmd[i] = Wire.read();
    }
    hexDump(F("Command"), cmd, length);

    switch (address >> 1u) {
        case ddcPriAddress:
        case ddcSecAddress:
        case ddcSpAddress:
            if (length != 1) {
                SerialDebug.print(F("**** Error: Invalid DDC command ****"));
                return;
            }
            setDdcOffset(address, cmd[0]);
            break;

        default:
            SerialDebug.println(F("**** Error: Invalid DDC address ****"));
            return;
    }
}

void receiveDdcReadRequest() {
    uint8_t address = TWDR;

    switch (address >> 1u) {
        case ddcPriAddress:
            SerialDebug.println(F("*** Primary DDC address received read request ***"));
            break;
        case ddcSecAddress:
            SerialDebug.println(F("*** Secondary DDC address received read request ***"));
            break;
        default:
            SerialDebug.println(F("*** Invalid DDC address ***"));
            return;
    }

    SerialDebug.println(F("*** Retrieving data from EEPROM ***"));
    uint8_t ddcData[edidUnitSize];
    if (!readDdcData(ddcData, address)) {
        SerialDebug.println(F("*** Failed to retrieve data from EEPROM ***"));
        return;
    }

    SerialDebug.println(F("*** Sending data to DDC host ***"));
    int bytesSent = 0;

    while (bytesSent < edidUnitSize) {
        int i = Wire.write(&ddcData[bytesSent], (edidUnitSize - bytesSent));

        SerialDebug.print(F("** Sent "));
        SerialDebug.print(i);
        SerialDebug.println(F(" bytes to DDC host **"));
        bytesSent += i;
    }
    SerialDebug.println(F("*** Transfer complete ***"));

    // Reset segment pointer
    ddcSegmentPointer = 0;

    // Flip word offset counter
    switch (address >> 1u) {
        case ddcPriAddress:
            ddcPriWordOffset += 0x80;
            break;
        case ddcSecAddress:
            ddcSecWordOffset += 0x80;
            break;
    }
}

bool setDdcOffset(uint8_t address, uint8_t offset) {
    switch (address >> 1u) {
        case ddcPriAddress:
        case ddcSecAddress:
            if (offset != 0x00 && offset != 0x80) {
                SerialDebug.print(F("*** Invalid DDC word offset: 0x"));
                SerialDebug.print(offset, HEX);
                SerialDebug.println(F(" ***"));
                return false;
            }
            break;

        case ddcSpAddress:
            if (offset > 0x7F) {
                SerialDebug.print(F("*** Invalid DDC segment pointer: 0x"));
                SerialDebug.print(offset, HEX);
                SerialDebug.println(F(" ***"));
                return false;
            }
            break;

        default:
            SerialDebug.println(F("*** Invalid DDC address ***"));
            return false;
    }

    switch (address >> 1u) {
        case ddcPriAddress:
            ddcPriWordOffset = offset;
            SerialDebug.print(F("*** Primary DDC word offset set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
        case ddcSecAddress:
            ddcSecWordOffset = offset;
            SerialDebug.print(F("*** Secondary DDC word offset set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
        case ddcSpAddress:
            ddcSegmentPointer = offset;
            SerialDebug.print(F("*** DDC segment pointer set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
    }

    return true;
}

bool verifyEdidCheckSum(uint8_t *data, int dataSize) {
    uint8_t checkSum = 0;

    for (int i = 0; i < dataSize; i++) {
        checkSum += data[i];
    }

    return checkSum == 0;
}