#include <ddc.h>
#include <debug.h>
#include <EEPROM.h>
#include <Wire.h>

// Variables
volatile uint8_t ddcSegmentPointer = 0;
volatile uint8_t ddcPriWordOffset = 0;
volatile uint8_t ddcSecWordOffset = 0;

uint8_t ddcPriMem[DDC_MEM_LENGTH];
size_t ddcPriMemUsed;
uint8_t ddcSecMem[DDC_MEM_LENGTH];
size_t ddcSecMemUsed;

void ddcDumpEdid() {
    uint8_t rxBuffer[DDC_EDID_LENGTH];

    SerialDebug.println(F("*** Reading sink primary EDID data ***"));

    // Read primary EDID data
    if (!ddcRequestEdid(rxBuffer, DDC_EDID_LENGTH)) {
        SerialDebug.println(F("*** Error reading EDID data ***"));
        return;
    }

    hexDump(rxBuffer, DDC_EDID_LENGTH);

    if (!ddcVerifyChecksum(rxBuffer, DDC_EDID_LENGTH)) {
        SerialDebug.println(F("*** Invalid checksum for sink primary EDID data ***"));
        return;
    }

    uint8_t numExtensions = rxBuffer[DDC_EDID_EXTEN_COUNT_POS];
    SerialDebug.print(F("*** EDID has "));
    SerialDebug.print(numExtensions, DEC);
    SerialDebug.println(F(" extensions ***"));

    for (uint8_t i = 1; i <= numExtensions; i++) {
        SerialDebug.print(F("*** Reading EDID extension "));
        SerialDebug.print(i, DEC);
        SerialDebug.println(F(" ***"));

        uint16_t offset = i * DDC_EDID_LENGTH;
        if (!ddcRequestEdid(rxBuffer, DDC_EDID_LENGTH, offset)) {
            SerialDebug.println(F("*** Error reading EDID extension "));
            SerialDebug.print(i, DEC);
            SerialDebug.println(F(" ***"));
            continue;
        }

        hexDump(rxBuffer, DDC_EDID_LENGTH);

        if (!ddcVerifyChecksum(&rxBuffer[offset], DDC_EDID_LENGTH)) {
            SerialDebug.print(F("*** Invalid checksum for EDID extension "));
            SerialDebug.print(i, DEC);
            SerialDebug.println(F(" ***"));
            continue;
        }
    }
}

size_t ddcLoadEeprom(uint8_t *buffer, uint16_t offset) {
    uint16_t length;
    EEPROM.get(offset, length);
    offset += 2; // skip first two bytes (length)

    // Limit the length to the reserved EEPROM size (-2 bytes for length)
    if (length > (DDC_MEM_LENGTH - 2)) {
        length = (DDC_MEM_LENGTH - 2);
    }

    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = EEPROM.read(offset + i);
    }

    return length;
}

bool ddcRequestEdid(uint8_t *data, uint8_t dataSize, uint16_t offset, uint8_t i2cAddress) {
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
    for (uint8_t i = 0; i < dataSize; i++) {
        if (i % DDC_EDID_LENGTH == 0) {
            Wire.requestFrom(i2cAddress, DDC_EDID_LENGTH);
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

void ddcReceiveCommand(int length) {
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
        case DDC_I2C_PRI_ADDR:
        case DDC_I2C_SEC_ADDR:
        case DDC_I2C_SP_ADDR:
            if (length != 1) {
                SerialDebug.print(F("**** Error: Invalid DDC command ****"));
                return;
            }
            ddcSetOffset(address, cmd[0]);
            break;

        default:
            SerialDebug.println(F("**** Error: Invalid DDC address ****"));
            return;
    }
}

void ddcReadRequest() {
    uint8_t address = TWDR;
    uint8_t *buffer;
    uint8_t wordOffset;
    size_t memUsed;

    switch (address >> 1u) {
        case DDC_I2C_PRI_ADDR:
            SerialDebug.println(F("**** Primary DDC address received read request ****"));
            buffer = ddcPriMem;
            memUsed = ddcPriMemUsed;
            wordOffset = ddcPriWordOffset;
            break;

        case DDC_I2C_SEC_ADDR:
            SerialDebug.println(F("**** Secondary DDC address received read request ****"));
            buffer = ddcSecMem;
            memUsed = ddcSecMemUsed;
            wordOffset = ddcSecWordOffset;
            break;

        default:
            SerialDebug.println(F("**** Invalid DDC address ****"));
            return;
    }

    SerialDebug.print(F("Word offset: 0x"));
    SerialDebug.println(wordOffset, HEX);

    SerialDebug.print(F("Segment pointer: 0x"));
    SerialDebug.println(ddcSegmentPointer, HEX);

    uint16_t totalOffset = wordOffset + (2 * DDC_EDID_LENGTH * ddcSegmentPointer);
    if (totalOffset > memUsed) {
        SerialDebug.println(F("**** Invalid memory position ****"));
        return;
    }

    uint16_t length = memUsed - totalOffset;
    if (length > DDC_EDID_LENGTH) {
        length = DDC_EDID_LENGTH;
    }

    buffer = &buffer[totalOffset];
    hexDump(buffer, length);

    SerialDebug.println(F("**** Sending data to DDC host ****"));
    if (length > DDC_EDID_LENGTH) {
        length = DDC_EDID_LENGTH;
    }
    uint8_t bytesSent = Wire.write(buffer, length);

    SerialDebug.print(F("**** Sent "));
    SerialDebug.print(bytesSent);
    SerialDebug.println(F(" bytes to DDC host ****"));

    // Reset segment pointer
    ddcSegmentPointer = 0;

    // Increment word offset
    switch (address >> 1u) {
        case DDC_I2C_PRI_ADDR:
            ddcPriWordOffset += bytesSent;
            break;
        case DDC_I2C_SEC_ADDR:
            ddcSecWordOffset += bytesSent;
            break;
    }
}

bool ddcSetOffset(uint8_t address, uint8_t offset) {
    switch (address >> 1u) {
        case DDC_I2C_PRI_ADDR:
        case DDC_I2C_SEC_ADDR:
            if (offset != 0x00 && offset != 0x80) {
                SerialDebug.print(F("*** Invalid DDC word offset: 0x"));
                SerialDebug.print(offset, HEX);
                SerialDebug.println(F(" ***"));
                return false;
            }
            break;

        case DDC_I2C_SP_ADDR:
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
        case DDC_I2C_PRI_ADDR:
            ddcPriWordOffset = offset;
            SerialDebug.print(F("*** Primary DDC word offset set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
        case DDC_I2C_SEC_ADDR:
            ddcSecWordOffset = offset;
            SerialDebug.print(F("*** Secondary DDC word offset set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
        case DDC_I2C_SP_ADDR:
            ddcSegmentPointer = offset;
            SerialDebug.print(F("*** DDC segment pointer set to 0x"));
            SerialDebug.print(offset, HEX);
            SerialDebug.print(F(" ***"));
            break;
    }

    return true;
}

void ddcSetup() {
    // Load EEPROM data
    SerialDebug.println(F("**** Loading DDC primary address data from EEPROM ****"));
    ddcPriMemUsed = ddcLoadEeprom(ddcPriMem);
    hexDump(ddcPriMem, ddcPriMemUsed);

    SerialDebug.println(F("**** Loading DDC secondary address data from EEPROM ****"));
    ddcSecMemUsed = ddcLoadEeprom(ddcSecMem, DDC_MEM_LENGTH);
    hexDump(ddcSecMem, ddcSecMemUsed);

    // Register as I2C slave
    SerialDebug.println(F("**** Registering as DDC sink ****"));
    Wire.begin(DDC_I2C_ALL_ADDR);
    TWAMR = DDC_I2C_ALL_MASK << 1u;

    // Disable SDA/SCL internal pull-up
    // DDC specifies host must pull-up SDA and SCL,
    // and sink must pull-up SCL  using a 47k ohm resistor
    digitalWrite(SDA, LOW);
    digitalWrite(SCL, LOW);

    Wire.onReceive(ddcReceiveCommand);
    Wire.onRequest(ddcReadRequest);
}

bool ddcVerifyChecksum(uint8_t *data, uint8_t dataSize) {
    uint8_t checksum = 0;

    for (uint8_t i = 0; i < dataSize; i++) {
        checksum += data[i];
    }

    return checksum == 0;
}