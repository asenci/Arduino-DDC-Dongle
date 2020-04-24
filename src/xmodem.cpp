#include <ascii.h>
#include <debug.h>
#include <EEPROM.h>
#include <xmodem.h>


// Variables
unsigned long lastNakSent = millis();

bool xModemDownload() {
    byte cmd;
    byte rxBuffer[xModemBlockSize];
    int blockPrevSeq = 0;

    while (true) {
        if (!xModemReadCmd(&cmd, 10000, 10, true)) {
            SerialDebug.println(F("* ERROR: XMODEM TRANSFER ABORTED AFTER 10 TIMEOUTS *"));
            return false;
        }

        SerialDebug.print(F("*** Received command from sender: 0x"));
        SerialDebug.print(cmd, HEX);
        SerialDebug.println(F(" ***"));

        switch(cmd) {
            case asciiCAN:
                SerialDebug.println(F("* ERROR: XMODEM TRANSFER CANCELLED BY SENDER *"));
                return false;

            case asciiEOT:
                // Send ACK
                Serial.write(asciiACK);
                return true;

            case asciiSOH: {
                rxBuffer[xModemBlockCmdPos] = cmd;
                if (!xModemReadBlock(rxBuffer)) {
                    SerialDebug.println(F("* ERROR: XMODEM ERROR RECEIVING DATA BLOCK *"));

                    xModemFlush();
                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                SerialDebug.println(F("*** Received payload via XMODEM ***"));
                hexDump(rxBuffer, xModemBlockSize);

                int blockSeq = rxBuffer[xModemBlockSeqPos];
                if (blockSeq == blockPrevSeq) {
                    SerialDebug.print(F("* WARNING: XMODEM DUPLICATE BLK SEQ: "));
                    SerialDebug.print(blockSeq, DEC);
                    SerialDebug.println(F(" *"));

                    xModemFlush();
                    // Send ACK
                    Serial.write(asciiACK);
                    continue;
                }
                if (blockSeq != (blockPrevSeq + 1)) {
                    SerialDebug.print(F("* ERROR: XMODEM INVALID BLK SEQ: "));
                    SerialDebug.print(blockSeq, DEC);
                    SerialDebug.println(F(" *"));

                    xModemFlush();

                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                int blockRevSeq = rxBuffer[xModemBlockRevSeqPos];
                if (blockRevSeq != (255 - blockSeq)) {
                    SerialDebug.print(F("* ERROR: XMODEM INVALID BLK REV SEQ: "));
                    SerialDebug.print(blockRevSeq, DEC);
                    SerialDebug.println(F(" *"));

                    xModemFlush();
                    // Send NAK

                    Serial.write(asciiNAK);
                    continue;
                }

                byte blockChecksum = rxBuffer[xModemBlockChecksumPos];
                byte calcChecksum = 0;

                for (int i = xModemBlockPayloadPos; i < xModemBlockChecksumPos; i++) {
                    calcChecksum += rxBuffer[i];
                }

                if (calcChecksum != blockChecksum) {
                    SerialDebug.print(F("* ERROR: XMODEM INVALID BLK CKSUM: 0x"));
                    SerialDebug.print(blockChecksum, HEX);
                    SerialDebug.println(F(" *"));

                    xModemFlush();

                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                // Checksum is valid, write to EEPROM
                for (int i = 0; i < xModemBlockPayloadSize; i++) {
                    int blockOffset = xModemBlockPayloadPos + i;
                    int eepromOffset = ((blockSeq - 1) * xModemBlockPayloadSize) + i;
                    EEPROM.write(eepromOffset, rxBuffer[blockOffset]);
                }

                blockPrevSeq = blockSeq;

                // Send ACK
                Serial.write(asciiACK);
                break;
            }

            default:
                SerialDebug.println(F("* ERROR: INVALID XMODEM COMMAND *"));
                Serial.write(asciiCAN);
                return false;
        }
    }

}

void xModemFlush() {
    SerialDebug.println(F("*** Flushing data on the serial buffer ***"));

    byte devNull;

    Serial.setTimeout(1000);
    for (int i = 0; Serial.readBytes(&devNull, 1); i++) {

        // Print received data
        if (i > 0) {
            if (i % 16 == 0) {
                SerialDebug.println();
            }
            else {
                SerialDebug.print(F(" "));
            }
        }

        if (devNull < 0x10) {
            SerialDebug.print(F("0"));
        }

        SerialDebug.print(devNull, HEX);

        SerialDebug.println();
    }
}

void xModemMainLoop(long nakInterval) {
    while (Serial.available()) {
        byte cmd = Serial.peek();

        SerialDebug.print(F("**** Received xModem command from UART: 0x"));
        SerialDebug.print(cmd, HEX);
        SerialDebug.println(F(" ****"));

        switch (cmd) {
            case asciiSOH:
                SerialDebug.println(F("**** Starting xModem download ****"));
                if (!xModemDownload()) {
                    SerialDebug.println(F("**** Download failed ****"));
                }
                break;

            case asciiNAK:
                SerialDebug.println(F("**** Starting xModem upload ****"));
                if (!xModemUpload()) {
                    SerialDebug.println(F("**** Upload failed ****"));
                }
                break;

            default:
                SerialDebug.println(F("* ERROR: INVALID UART COMMAND *"));

                // consume cmd
                Serial.read();
                break;
        }
    }

    // Send NAK every 10s to advertise xModem
    unsigned long curMillis = millis();
    if (curMillis - lastNakSent > nakInterval) {
        if (Serial.availableForWrite()) {
            Serial.write(asciiNAK);
        }

        lastNakSent = curMillis;
    }
}

bool xModemReadBlock(byte *data) {
    Serial.setTimeout(1000);

    // skip first byte (cmd)
    int receivedBytes = Serial.readBytes(&data[1], xModemBlockSize - 1);

    return receivedBytes == (xModemBlockSize - 1);
}

bool xModemReadCmd(byte *data, unsigned long timeout, int tries, bool nak) {
    Serial.setTimeout(timeout);

    for (int i = 0; i < tries; i++) {
        // Wait for command
        int receivedBytes = Serial.readBytes(data, 1);

        if (receivedBytes == 1) {
            return true;
        }

        SerialDebug.println(F("* ERROR: XMODEM TIMEOUT *"));

        if (nak) {
            Serial.write(asciiNAK);
        }
    }

    SerialDebug.println(F("* ERROR: XMODEM TIMED OUT WAITING FOR CMD *"));
    return false;
}

bool xModemUpload() {
    byte cmd;
    byte blockSeq = 0;

    while (true) {
        if (!xModemReadCmd(&cmd, 60000)) {
            SerialDebug.println(F("* ERROR: XMODEM TRANSFER TIMED OUT *"));
            return false;
        }

        switch (cmd) {
            case asciiACK:
                if (blockSeq * xModemBlockPayloadSize >= (int)EEPROM.length()) {
                    // Receiver acknowledged EOT, transfer completed
                    return true;
                }

                // Send next block
                blockSeq++;
                break;

            case asciiCAN:
                SerialDebug.println(F("* ERROR: XMODEM TRANSFER CANCELLED BY RECEIVER *"));
                return false;

            case asciiNAK:
                if (blockSeq > 0) {
                    SerialDebug.println(F("* WARNING: XMODEM NAK RECEIVED, RETRANSMITING BLOCK *"));
                }
                break;

            default:
                SerialDebug.println(F("* ERROR: INVALID XMODEM COMMAND, ABORTING TRANSFER *"));
                Serial.write(asciiCAN);
                return false;
        }

        if (blockSeq * xModemBlockPayloadSize >= (int)EEPROM.length()) {
            // Last block has been sent, sending EOT and waiting for ACK
            Serial.write(asciiEOT);
            continue;
        }

        // Send data block
        Serial.write(asciiSOH);
        Serial.write(blockSeq+1); // xModem block sequence starts from 1
        Serial.write(255-blockSeq-1);

        byte checksum = 0;

        for (int i = 0; i < xModemBlockPayloadSize; i++) {
            byte b = asciiSUB;

            int idx = (blockSeq * xModemBlockPayloadSize) + i;
            if (idx < (int) EEPROM.length()) {
                b = EEPROM[idx];
            }

            Serial.write(b);
            checksum += b;
        }

        Serial.write(checksum);
    }
}