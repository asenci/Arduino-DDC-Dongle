#include <ascii.h>
#include <xmodem.h>

bool xModemReadCmd(byte *data) {
    int receivedBytes = 0;

    Serial.setTimeout(10000);

    for (int i = 0; i < 10; i++) {
        // Wait for command
        receivedBytes = Serial.readBytes(data, 1);

        // Read successful
        if (receivedBytes == 1) {
            return true;
        }

        // Send NAK
        Serial.write(asciiNAK);

#ifdef DEBUG
        Serial.println("* ERROR: XMODEM TRANSFER TIMED OUT *");
#endif
    }

    return false;
}

bool xModemReadBlock(byte *data) {
    Serial.setTimeout(1000);

    // skip first byte (cmd)
    int receivedBytes = Serial.readBytes(&data[1], xModemBlockSize - 1);

    return receivedBytes == (xModemBlockSize - 1);
}

void xModemFlush() {
    byte devNull;

    Serial.setTimeout(1000);

#ifdef DEBUG
    Serial.println("*** Flushing data on the serial buffer ***");
#endif

    for (int i = 0; Serial.readBytes(&devNull, 1); i++) {
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

        if (devNull < 0x10) {
            Serial.print("0");
        }

        Serial.print(devNull, HEX);

        digitalWrite(LED_BUILTIN, LOW);
#endif
    }
#ifdef DEBUG
    Serial.println();
#endif
}
