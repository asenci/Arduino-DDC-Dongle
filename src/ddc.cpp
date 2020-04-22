#include <Arduino.h>
#include <ddc.h>
#include <EEPROM.h>
#include <Wire128.h>


volatile int curOffset = 0;


void receiveDdcCommand(int numBytes) {
#ifdef DEBUG
    Serial.print("*** Receiving ");
    Serial.print(numBytes, DEC);
    Serial.println(" bytes ***");
#endif

    for (int i = 0; Wire.available(); i++) {
        byte cmd = Wire.read();

        // Received word offset from host, adjust cursor
        if (i == 0 && numBytes == 1) {
            curOffset = cmd;
        } else if (i == 0) {
#ifdef DEBUG
            Serial.println("*** Received unsupported command from host ***");
#endif
        }

#ifdef DEBUG
        digitalWrite(LED_BUILTIN, HIGH);

    // Print received data
    if (i > 0) {
      if (i % 16 == 0) {
        Serial.println();
      }
      else {
        Serial.print(" ");
      }
    }

    if (cmd < 0x10) {
      Serial.print("0");
    }

    Serial.print(cmd, HEX);

    digitalWrite(LED_BUILTIN, LOW);
#endif
    }

#ifdef DEBUG
    Serial.println();
#endif
}


void sendDdcData() {
    byte buffer[edidUnitSize];

#ifdef DEBUG
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print("*** Received read request, sending ");
    Serial.print(edidUnitSize, DEC);
    Serial.print(" bytes from offset 0x");
    Serial.print(curOffset, HEX);
    Serial.println(" ***");

    for (int i = 0; i < edidUnitSize; i++) {
        if (i > 0) {
            if (i % 16 == 0) {
              Serial.println();
            }
            else {
              Serial.print(" ");
            }
        }

        if (EEPROM[curOffset+i] < 0x10) {
            Serial.print("0");
        }

        Serial.print(EEPROM[curOffset+i], HEX);
    }
    Serial.println();
#endif

    EEPROM.get(curOffset, buffer);

    // Send to host
    int bytesSent = Wire.write(buffer, edidUnitSize);

    // Advance cursor by the number of sent bytes
    curOffset += bytesSent;

    // Reset cursor if overflow
    if (curOffset > (int) EEPROM.length()) {
        curOffset = 0;
    }

#ifdef DEBUG
    Serial.print("*** Sent ");
    Serial.print(bytesSent, DEC);
    Serial.println(" bytes to host ***");

    digitalWrite(LED_BUILTIN, LOW);
#endif
}


void dumpEDID() {
    byte edidData[edidUnitSize];

    // E-DDC specifies monitor must be ready to reply to host within 20ms
    delay(20);

    Serial.println("*** Reading monitor primary EDID data ***");

    // Read primary EDID data
    if (!readAtOffset(edidData, edidUnitSize)) {
        Serial.println("*** Error reading EDID data ***");
        return;
    }

    hexDump(edidData, edidUnitSize);

    if (!verifyCheckSum(edidData, edidUnitSize)) {
        Serial.println("*** Invalid checksum for monitor primary EDID data ***");
        return;
    }

    int numExtensions = edidData[edidNumExtPos];
    Serial.print("*** EDID has ");
    Serial.print(numExtensions, DEC);
    Serial.println(" extensions ***");

    for (int i = 1; i <= numExtensions; i++) {
        Serial.print("*** Reading EDID extension ");
        Serial.print(i, DEC);
        Serial.println(" ***");

        int offset = i*edidUnitSize;
        if (!readAtOffset(edidData, edidUnitSize, offset)) {
            Serial.println("*** Error reading EDID extension ");
            Serial.print(i, DEC);
            Serial.println(" ***");
            continue;
        }

        hexDump(edidData, edidUnitSize);

        if (!verifyCheckSum(&edidData[offset], edidUnitSize)) {
            Serial.print("*** Invalid checksum for EDID extension ");
            Serial.print(i, DEC);
            Serial.println(" ***");
            continue;
        }
    }
}

bool readAtOffset(byte *data, int dataSize) {
    return readAtOffset(data, dataSize, 0);
}

bool readAtOffset(byte *data, int dataSize, int offset) {
    return readAtOffset(data, dataSize, offset, ddcPriAddress);
}

bool readAtOffset(byte *data, int dataSize, int offset, int i2cAddress) {
    Serial.print("*** Reading ");
    Serial.print(dataSize, DEC);
    Serial.print(" bytes from address 0x");
    Serial.print(i2cAddress << 1, HEX);
    Serial.print(" at offset 0x");
    Serial.print(offset, HEX);
    Serial.println(" ***");

    // Set word offset
    Wire.beginTransmission(i2cAddress);
    Wire.write(offset);
    Wire.endTransmission();


    // Read EDID data
    for (int i = 0; i < dataSize; i++) {
        // Read 32 bytes at a time due to Wire buffer limitation
        if (i % 32 == 0) {
            Wire.requestFrom(i2cAddress, 32);
        }

        if (Wire.available() < 1) {
            Serial.println("*ERROR: NO DATA AVAILABLE*");
            return false;
        }

        digitalWrite(LED_BUILTIN, HIGH);
        data[i] = Wire.read();
        digitalWrite(LED_BUILTIN, LOW);
    }

    // Empty the buffer
    if (Wire.available() > 0) {
        Serial.println("*ERROR: BUFFER OVERFLOW*");
    }

    return true;
}

bool hexDump(byte *data, int dataSize) {
    for (int i = 0; i < dataSize; i++) {
        if (i > 0) {
            if (i % 16 == 0) {
                Serial.println();
            }
            else {
                Serial.print(" ");
            }
        }

        if (data[i] < 0x10) {
            Serial.print("0");
        }

        Serial.print(data[i], HEX);
    }
    Serial.println();
    return true;
}

bool verifyCheckSum(byte *data, int dataSize) {
    byte checkSum = 0;

    for (int i = 0; i < dataSize; i++) {
        checkSum += data[i];
    }

    return checkSum == 0;
}