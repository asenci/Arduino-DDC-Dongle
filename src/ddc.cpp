#include <Arduino.h>
#include <ddc.h>
#include <debug.h>
#include <EEPROM.h>
#include <Wire128.h>


// Variables
volatile int curOffset = 0;


void receiveDdcCommand(int numBytes) {
    SerialDebug.print(F("*** Receiving "));
    SerialDebug.print(numBytes, DEC);
    SerialDebug.println(F(" bytes ***"));

    byte cmd[numBytes];

    for (int i = 0; i < numBytes; i++) {
        cmd[i] = Wire.read();
    }

    SerialDebug.println(F("*** Received command from host ***"));
    hexDump(cmd, numBytes);

    if (numBytes != 1) {
        SerialDebug.println("* ERROR: INVALID DDC COMMAND *");
        return;
    }

    curOffset = cmd[0];
}


void sendDdcData() {
    byte txBuffer[edidUnitSize];

    SerialDebug.print(F("*** Received read request, sending "));
    SerialDebug.print(edidUnitSize, DEC);
    SerialDebug.print(F(" bytes from offset 0x"));
    SerialDebug.print(curOffset, HEX);
    SerialDebug.println(F(" ***"));

    EEPROM.get(curOffset, txBuffer);

    hexDump(txBuffer, edidUnitSize);

    int bytesRemaining = edidUnitSize;
    int bytesSent = 0;

    while (bytesSent < bytesRemaining) {
        int i = Wire.write(&txBuffer[bytesSent], bytesRemaining);

        bytesSent += i;
        bytesRemaining -= i;
    }

    // Advance cursor by the number of sent bytes
    curOffset += bytesSent;

    // Reset cursor if overflow
    if (curOffset > (int) EEPROM.length()) {
        curOffset = 0;
    }

    SerialDebug.print(F("*** Sent "));
    SerialDebug.print(bytesSent, DEC);
    SerialDebug.println(F(" bytes to host ***"));
}


void dumpEDID() {
    byte rxBuffer[edidUnitSize];

    SerialDebug.println(F("*** Reading sink primary EDID data ***"));

    // Read primary EDID data
    if (!readEdidAtOffset(rxBuffer, edidUnitSize)) {
        SerialDebug.println(F("*** Error reading EDID data ***"));
        return;
    }

    hexDump(rxBuffer, edidUnitSize);

    if (!verifyEdidCheckSum(rxBuffer, edidUnitSize)) {
        SerialDebug.println(F("*** Invalid checksum for sink primary EDID data ***"));
        return;
    }

    int numExtensions = rxBuffer[edidNumExtPos];
    SerialDebug.print(F("*** EDID has "));
    SerialDebug.print(numExtensions, DEC);
    SerialDebug.println(F(" extensions ***"));

    for (int i = 1; i <= numExtensions; i++) {
        SerialDebug.print(F("*** Reading EDID extension "));
        SerialDebug.print(i, DEC);
        SerialDebug.println(F(" ***"));

        int offset = i*edidUnitSize;
        if (!readEdidAtOffset(rxBuffer, edidUnitSize, offset)) {
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


bool readEdidAtOffset(byte *data, int dataSize, int offset, int i2cAddress) {
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


bool verifyEdidCheckSum(byte *data, int dataSize) {
    byte checkSum = 0;

    for (int i = 0; i < dataSize; i++) {
        checkSum += data[i];
    }

    return checkSum == 0;
}