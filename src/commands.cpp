#include <Arduino.h>
#include <ascii.h>
#include <commands.h>
#include <EEPROM.h>
#include <xmodem.h>

bool processCommand(byte cmd) {
    switch (cmd) {
        case asciiSOH:
#ifdef DEBUG
            Serial.println("*** Starting data transfer from UART to EEPROM ***");
#endif
            if (!receiveEEPROM()) {
#ifdef DEBUG
                Serial.println("*** Data transfer failed ***");
#endif
                return false;
            }
            break;

        case asciiNAK:
#ifdef DEBUG
            Serial.println("*** Starting data transfer from EEPROM to UART ***");
#endif
            if (!sendEEPROM()) {
#ifdef DEBUG
                Serial.println("*** Data transfer failed ***");
#endif
                return false;
            }
            break;

        default:
#ifdef DEBUG
            Serial.println("* ERROR: INVALID UART COMMAND *");
#endif
            return false;
            break;
    }
    
    return true;
}


bool receiveEEPROM() {
    byte cmd;
    byte blockData[xModemBlockSize];
    int blockPrevSeq = 0;

    // Send initial NAK
    Serial.write(asciiNAK);

    while (true) {
        if (!xModemReadCmd(&cmd)) {
#ifdef DEBUG
            Serial.println("* ERROR: XMODEM TRANSFER ABORTED AFTER 10 TIMEOUTS *");
#endif

            return false;
        }

#ifdef DEBUG
        Serial.print("*** Received command from sender: 0x");
        Serial.print(cmd, HEX);
        Serial.println(" ***");
#endif

        switch(cmd) {
            case asciiCAN:
#ifdef DEBUG
                Serial.println("* ERROR: XMODEM TRANSFER CANCELLED BY SENDER *");
#endif
                return false;

            case asciiEOT:
                // Send ACK
                Serial.write(asciiACK);
                return true;

            case asciiSOH: {
                blockData[xModemBlockCmdPos] = cmd;
                if (!xModemReadBlock(blockData)) {
#ifdef DEBUG
                    Serial.println("* ERROR: XMODEM ERROR RECEIVING DATA BLOCK *");
#endif

                    xModemFlush();
                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

#ifdef DEBUG
                Serial.println("*** Received payload via XMODEM ***");
                for (int i = 0; i < xModemBlockSize; i++) {
                    digitalWrite(LED_BUILTIN, HIGH);

                    byte b = blockData[i];

                    // Print received data
                    if (i > 0) {
                        if (i % 16 == 0) {
                            Serial.println();
                        }
                        else {
                            Serial.print(" ");
                        }
                    }

                    if (b < 0x10) {
                        Serial.print("0");
                    }

                    Serial.print(b, HEX);

                    digitalWrite(LED_BUILTIN, LOW);
                }
                Serial.println();
#endif

                int blockSeq = blockData[xModemBlockSeqPos];
                if (blockSeq == blockPrevSeq) {
#ifdef DEBUG
                    Serial.print("* WARNING: XMODEM DUPLICATE BLK SEQ: ");
                    Serial.print(blockSeq, DEC);
                    Serial.println(" *");
#endif

                    xModemFlush();
                    // Send ACK
                    Serial.write(asciiACK);
                    continue;
                }
                if (blockSeq != (blockPrevSeq + 1)) {
#ifdef DEBUG
                    Serial.print("* ERROR: XMODEM INVALID BLK SEQ: ");
                    Serial.print(blockSeq, DEC);
                    Serial.println(" *");
#endif

                    xModemFlush();
                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                int blockRevSeq = blockData[xModemBlockRevSeqPos];
                if (blockRevSeq != (255 - blockSeq)) {
#ifdef DEBUG
                    Serial.print("* ERROR: XMODEM INVALID BLK REV SEQ: ");
                    Serial.print(blockRevSeq, DEC);
                    Serial.println(" *");
#endif

                    xModemFlush();
                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                byte blockChecksum = blockData[xModemBlockChecksumPos];
                byte calcChecksum = 0;

                for (int i = xModemBlockPayloadPos; i < xModemBlockChecksumPos; i++) {
                    calcChecksum += blockData[i];
                }

                if (calcChecksum != blockChecksum) {
#ifdef DEBUG
                    Serial.print("* ERROR: XMODEM INVALID BLK CKSUM: 0x");
                    Serial.print(blockChecksum, HEX);
                    Serial.println(" *");
#endif

                    xModemFlush();
                    // Send NAK
                    Serial.write(asciiNAK);
                    continue;
                }

                // Checksum is valid, write to EEPROM
                for (int i = 0; i < xModemBlockPayloadSize; i++) {
                    int blockOffset = xModemBlockPayloadPos + i;
                    int eepromOffset = ((blockSeq - 1) * xModemBlockPayloadSize) + i;
                    EEPROM.write(eepromOffset, blockData[blockOffset]);
                }

                blockPrevSeq = blockSeq;

                // Send ACK
                Serial.write(asciiACK);
                break;
            }

            default:
#ifdef DEBUG
                Serial.println("* ERROR: INVALID XMODEM COMMAND *");
#endif
                break;
        }
    }

    return false;
}

bool sendEEPROM() {
    return true;
}
