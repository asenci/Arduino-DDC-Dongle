#include <debug.h>
#include <pins.h>
#include <SoftwareSerial.h>

// Initialize software serial port for debugging
SoftwareSerial SerialDebug(debugRxPin, debugTxPin);


void dumpByte(uint8_t b) {
    if (b < 0x10) {
        SerialDebug.print(F("0"));
    }

    SerialDebug.print(b, HEX);
}

void hexDump(uint8_t *data, int length, int indent) {
    for (int i = 0; i < length; i++) {
        if (i % 16 == 0) {
            if (i > 0) {
                SerialDebug.println();
            }
            for (int j = 0; j < indent; j++) {
                SerialDebug.print(F(" "));
            }
        } else {
            SerialDebug.print(F(" "));
        }

        dumpByte(data[i]);
    }
    SerialDebug.println();
}

void hexDump(const __FlashStringHelper *name, uint8_t *data, int length, int indent) {
    for (int i = 0; i < indent; i++) {
        SerialDebug.print(" ");
    }
    SerialDebug.print(name);

    if (length == 1) {
        SerialDebug.print(F(": "));
        SerialDebug.print(*data, DEC);
        SerialDebug.print(F(" (0x"));
        dumpByte(*data);
        SerialDebug.println(F(")"));
    } else {
        SerialDebug.println(F(":"));
        hexDump(data, length, indent+1);
    }

}
