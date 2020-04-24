#include <pins.h>
#include <SoftwareSerial.h>

// Initialize software serial port for debugging
SoftwareSerial SerialDebug(debugRxPin, debugTxPin);

bool hexDump(byte *data, int dataSize) {
    for (int i = 0; i < dataSize; i++) {
        if (i > 0) {
            if (i % 16 == 0) {
                SerialDebug.println();
            }
            else {
                SerialDebug.print(F(" "));
            }
        }

        if (data[i] < 0x10) {
            SerialDebug.print(F("0"));
        }

        SerialDebug.print(data[i], HEX);
    }
    SerialDebug.println();
    return true;
}

