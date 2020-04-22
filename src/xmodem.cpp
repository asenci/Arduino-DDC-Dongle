#include <ascii.h>
#include <xmodem.h>

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

bool xModemReadBlock(byte *data) {
    Serial.setTimeout(1000);

    // skip first byte (cmd)
    int receivedBytes = Serial.readBytes(&data[1], xModemBlockSize - 1);

    return receivedBytes == (xModemBlockSize - 1);
}

bool xModemReadCmd(byte *data) {
    return xModemReadCmd(data, 10000);
}

bool xModemReadCmd(byte *data, unsigned long timeout) {
    return xModemReadCmd(data, timeout, 1);
}

bool xModemReadCmd(byte *data, unsigned long timeout, int tries) {
    int receivedBytes;

    Serial.setTimeout(timeout);

    for (int i = 0; i < tries; i++) {
        // Wait for command
        receivedBytes = Serial.readBytes(data, 1);

        if (receivedBytes == 1) {
            return true;
        }

#ifdef DEBUG
        Serial.println("* ERROR: XMODEM TIMEOUT *");
#endif
        if (tries > 1) {
            // Send NAK
            Serial.write(asciiNAK);
        }
    }

#ifdef DEBUG
    Serial.println("* ERROR: XMODEM TIMED OUT WAITING FOR CMD *");
#endif
    return false;
}
