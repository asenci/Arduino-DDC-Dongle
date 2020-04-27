#include <ascii.h>
#include <debug.h>
#include <EEPROM.h>
#include <xmodem.h>


// Variables
unsigned long lastNakSent = millis();

bool xModemDownload() {
    uint8_t cmd;
    uint8_t blockSeq = 1;

    while (true) {
        if (!xModemReadByte(&cmd, 1, 10000, 10, true)) {
            SerialDebug.println(F("*** Timed out waiting for data ***"));
            return false;
        }

        switch (cmd) {
            case CAN:
                SerialDebug.println(F("*** Received CAN, aborting transfer ***"));
                return false;

            case EOT:
                SerialDebug.println(F("*** Received EOT, transfer completed, sending ACK ***"));
                Serial.write(ACK);
                return true;

            case SOH: {
                SerialDebug.println(F("*** Received SOH, reading data block ***"));
                uint8_t blockPayload[XMODEM_PAYLOAD_LENGTH];
                uint8_t result = xModemReadBlock(blockPayload, blockSeq);

                if (result < 0) {
                    SerialDebug.println(F("*** Invalid data block, sending NAK ***"));
                    Serial.write(NAK);
                    continue;
                }

                if (result == 0) {
                    SerialDebug.println(F("*** Retransmitted data block, sending ACK ***"));
                    Serial.write(ACK);
                    continue;
                }

                // Block is valid and not re-transmission, write to EEPROM
                SerialDebug.println(F("*** Valid data block, writing payload to EEPROM ***"));
                uint16_t eepromOffset = (blockSeq - 1) * XMODEM_PAYLOAD_LENGTH;
                EEPROM.put(eepromOffset, blockPayload);

                SerialDebug.println(F("*** Payload written to EEPROM, sending ACK ***"));
                Serial.write(ACK);
                blockSeq += result;
                break;
            }

            default:
                SerialDebug.print(F("*** Received invalid xModem command: 0x"));
                SerialDebug.print(cmd, HEX);
                SerialDebug.println(F(" , sending CAN ***"));
                Serial.write(CAN);
                return false;
        }
    }
}

void xModemMainLoop(unsigned long nakInterval) {
    while (Serial.available()) {
        uint8_t cmd = Serial.peek();

        switch (cmd) {
            case SOH:
                SerialDebug.println(F("**** Starting xModem download ****"));
                if (!xModemDownload()) {
                    SerialDebug.println(F("**** Download failed ****"));
                } else {
                    SerialDebug.println(F("**** Download completed, resetting ****"));
                    WDTCSR = 0x18u;
                }
                break;

            case NAK:
                SerialDebug.println(F("**** Starting xModem upload ****"));
                if (!xModemUpload()) {
                    SerialDebug.println(F("**** Upload failed ****"));
                } else {
                    SerialDebug.println(F("**** Upload completed ****"));
                }
                break;

            default:
                SerialDebug.print(F("**** Received invalid command: 0x"));
                SerialDebug.print(cmd, HEX);
                SerialDebug.println(F(" ****"));
                Serial.read(); // consume cmd
                break;
        }
    }

    // Send NAK periodically to advertise xModem to receiver
    unsigned long curMillis = millis();
    if (curMillis - lastNakSent > nakInterval) {
        if (Serial.availableForWrite()) {
            SerialDebug.println(F("**** Sending periodic NAK ****"));
            Serial.write(NAK);
        }

        lastNakSent = curMillis;
    }
}

uint8_t xModemReadBlock(uint8_t *buffer, uint8_t expectedBlockSeq) {
    //      Each block of the transfer looks like:
    //            <SOH><blk #><255-blk #><--128 data bytes--><cksum>
    //      in which:
    //      <SOH>           = 01 hex
    //      <blk #>         = binary number, starts at 01 increments by 1, and
    //                wraps 0FFH to 00H (not to 01)
    //      <255-blk #>   =   blk # after going thru 8080 "CMA" instr, i.e.
    //                each bit complemented in the 8-bit block number.
    //                Formally, this is the "ones complement".
    //      <cksum>       = the sum of the data bytes only.  Toss any carry.

    // Read block structure before processing

    uint8_t blockSeq;
    if (!xModemReadByte(&blockSeq)) {
        return -1;
    }
    hexDump(F("SEQ"), &blockSeq);

    uint8_t blockRevSeq;
    if (!xModemReadByte(&blockRevSeq)) {
        return -1;
    }
    hexDump(F("REV SEQ"), &blockRevSeq);

    if (!xModemReadByte(buffer, XMODEM_PAYLOAD_LENGTH)) {
        return -1;
    }
    hexDump(F("PAYLOAD"), buffer, XMODEM_PAYLOAD_LENGTH);

    uint8_t blockChecksum;
    if (!xModemReadByte(&blockChecksum)) {
        return -1;
    }
    hexDump(F("CKSUM"), &blockChecksum);

    // Process block data

    if (expectedBlockSeq > 1 && blockSeq == expectedBlockSeq - 1) {
        SerialDebug.print(F("* WARNING: XMODEM DUPLICATE BLK SEQ: "));
        SerialDebug.print(blockSeq, DEC);
        SerialDebug.println(F(" *"));
        return 0;
    }

    if (blockSeq != (expectedBlockSeq)) {
        SerialDebug.print(F("* ERROR: XMODEM INVALID BLK SEQ: "));
        SerialDebug.print(blockSeq, DEC);
        SerialDebug.println(F(" *"));
        return -1;
    }

    if (blockRevSeq != (255 - blockSeq)) {
        SerialDebug.print(F("* ERROR: XMODEM INVALID BLK REV SEQ: "));
        SerialDebug.print(blockRevSeq, DEC);
        SerialDebug.println(F(" *"));
        return -1;
    }

    uint8_t calcChecksum = 0;
    for (uint8_t i = 0; i < XMODEM_PAYLOAD_LENGTH; i++) {
        calcChecksum += buffer[i];
    }
    if (calcChecksum != blockChecksum) {
        SerialDebug.print(F("* ERROR: XMODEM INVALID BLK CKSUM: 0x"));
        SerialDebug.print(blockChecksum, HEX);
        SerialDebug.println(F(" *"));
        return -1;
    }

    return 1;
}

bool xModemReadByte(uint8_t *buffer, size_t length, unsigned long timeout, uint8_t tries, bool nak) {
    Serial.setTimeout(timeout);

    for (uint8_t i = 0; i < tries; i++) {
        // Wait for command
        size_t receivedBytes = Serial.readBytes(buffer, length);

        if (receivedBytes == length) {
            return true;
        }

        if (nak) {
            SerialDebug.println(F("** Timed out, sending NAK **"));
            Serial.write(NAK);
        }
    }

    return false;
}

void xModemSendBlock(uint8_t *buffer, uint8_t blockSeq) {
    SerialDebug.println(F("** Sending SOH **"));
    Serial.write(SOH);

    hexDump(F("SEQ"), &blockSeq);
    Serial.write(blockSeq);

    uint8_t revBlockSeq = 255 - blockSeq;
    hexDump(F("REV SEQ"), &revBlockSeq);
    Serial.write(revBlockSeq);

    uint8_t checksum = 0;

    hexDump(F("PAYLOAD"), buffer, XMODEM_PAYLOAD_LENGTH);
    for (uint8_t i = 0; i < XMODEM_PAYLOAD_LENGTH; i++) {
        Serial.write(buffer[i]);
        checksum += buffer[i];
    }

    hexDump(F("CKSUM"), &checksum);
    Serial.write(checksum);
}

bool xModemUpload() {
    uint8_t cmd;
    uint8_t blockSeq = 1;
    bool eotSent = false;

    while (true) {
        if (!xModemReadByte(&cmd, 1, 60000)) {
            SerialDebug.println(F("*** Timed out waiting for data ***"));
            return false;
        }

        switch (cmd) {
            case ACK:
                if (eotSent) {
                    // Receiver acknowledged EOT, transfer completed
                    SerialDebug.println(F("*** Received ACK, transfer completed ***"));
                    return true;
                }

                SerialDebug.println(F("*** Received ACK, sending data block ***"));
                blockSeq++;
                break;

            case CAN:
                SerialDebug.println(F("*** Received CAN, aborting transfer ***"));
                return false;

            case NAK:
                if (blockSeq == 1) {
                    SerialDebug.println(F("*** Received initial NAK, starting transfer ***"));
                } else {
                    SerialDebug.println(F("*** Received NAK, re-sending data block ***"));
                }
                break;

            default:
                SerialDebug.print(F("*** Received invalid xModem command: 0x"));
                SerialDebug.print(cmd, HEX);
                SerialDebug.println(F(" , sending CAN ***"));
                Serial.write(CAN);
                return false;
        }

        uint16_t eepromOffset = (blockSeq - 1) * XMODEM_PAYLOAD_LENGTH;
        if (eepromOffset >= EEPROM.length()) {
            // Last block has been sent, sending EOT and waiting for ACK
            SerialDebug.println(F("*** Last data block sent, sending EOT ***"));
            Serial.write(EOT);
            eotSent = true;
            continue;
        }

        SerialDebug.println(F("*** Reading payload from EEPROM ***"));
        uint8_t blockPayload[XMODEM_PAYLOAD_LENGTH];
        EEPROM.get(eepromOffset, blockPayload);
        hexDump(blockPayload, XMODEM_PAYLOAD_LENGTH);


        uint16_t payloadLength = EEPROM.length() - eepromOffset;
        if (payloadLength > XMODEM_PAYLOAD_LENGTH) {
            payloadLength = XMODEM_PAYLOAD_LENGTH;
        }

        uint8_t paddingRequired = XMODEM_PAYLOAD_LENGTH - payloadLength;
        if (paddingRequired > 0) {
            SerialDebug.print(F("*** Padding data block with "));
            SerialDebug.print(paddingRequired, DEC);
            SerialDebug.println(F(" bytes of SUB ***"));
        }
        for (uint8_t i = 0; i < paddingRequired; i++) {
            blockPayload[XMODEM_PAYLOAD_LENGTH - i] = SUB;
        }

        SerialDebug.println(F("*** Sending data block ***"));
        xModemSendBlock(blockPayload, blockSeq);
        SerialDebug.println(F("*** Data block sent ***"));
    }
}