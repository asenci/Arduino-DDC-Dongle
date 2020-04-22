#include <EEPROM.h>
#include "src/Wire128/src/Wire.h"

// Enable debugging
// #define DEBUG


// Constants
const byte asciiSOH = 0x01;
const byte asciiEOT = 0x04;
const byte asciiACK = 0x06;
const byte asciiNAK = 0x15;
const byte asciiCAN = 0x18;

const int dataUnitSize = 128;

const int ddcPriAddress = 0xA0 >> 1;
const int ddcSecAddress = 0xA4 >> 1;
const int ddcSpAddress = 0x60 >> 1;

const int hotPlugDetectPin = 7;

const int xModemBlockSize = 132; // CMD byte is read in a separate routine
const int xModemBlockCmdPos = 0;
const int xModemBlockSeqPos = 1;
const int xModemBlockRevSeqPos = 2;
const int xModemBlockPayloadPos = 3;
const int xModemBlockPayloadSize = 128;
const int xModemBlockChecksumPos = xModemBlockSize-1;

// Variables
volatile int curOffset = 0;
byte rxBuffer[dataUnitSize];
byte txBuffer[dataUnitSize];


void setup() {
    Serial.begin(9600);

#ifdef DEBUG
    // Wait for serial interface to be ready
  while (!Serial) {}

  Serial.println("**** Ready ****");

  // Use built-in LED for status
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

    // Use hotplugPin for hotplug detection
    // Monitor must pull-up to a +5v reference to signal presence to the host
    pinMode(hotPlugDetectPin, OUTPUT);
    digitalWrite(hotPlugDetectPin, LOW);

    // Register i2c slave
    Wire.begin(ddcPriAddress);

    // Disable SDA/SCL pull-up
    // E-DDC specifies monitor must pull-up SCL to a +5v reference using a 47k ohm resistor
    digitalWrite(SDA, LOW);
    digitalWrite(SCL, LOW);

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

#ifdef DEBUG
    delay(200);

  Serial.println("**** Enabling hotplug ****");
#endif

    digitalWrite(hotPlugDetectPin, HIGH);
}

void loop() {
    while (Serial.available()) {
        byte cmd = Serial.read();
#ifdef DEBUG
        Serial.print("**** Received command from UART: 0x");
    Serial.print(cmd, HEX);
    Serial.println(" ****");
#endif
        switch (cmd) {
            case asciiSOH:
#ifdef DEBUG
                Serial.println("**** Starting data transfer from UART to EEPROM ****");
#endif
                if (!receiveEEPROM()) {
#ifdef DEBUG
                    Serial.println("**** Data transfer failed ****");
#endif
                }
                break;

            case asciiNAK:
#ifdef DEBUG
                Serial.println("**** Starting data transfer from EEPROM to UART ****");
#endif
                if (!sendEEPROM()) {
#ifdef DEBUG
                    Serial.println("**** Data transfer failed ****");
#endif
                }
                break;

            default:
#ifdef DEBUG
                Serial.println("* ERROR: INVALID UART COMMAND *");
#endif
                break;
        }
    }
}


void requestEvent() {
#ifdef DEBUG
    Serial.print("*** Received read request, sending ");
    Serial.print(dataUnitSize, DEC);
    Serial.print(" bytes from offset 0x");
    Serial.print(curOffset, HEX);
    Serial.println(" ***");
#endif

    // Read from EEPROM
    EEPROM.get(curOffset, txBuffer);

#ifdef DEBUG
    for (int i = 0; i < dataUnitSize; i++) {
        if (i > 0) {
            if (i % 16 == 0) {
              Serial.println();
            }
            else {
              Serial.print(" ");
            }
        }

        if (txBuffer[i] < 0x10) {
            Serial.print("0");
        }

        Serial.print(txBuffer[i], HEX);
  }
  Serial.println();
#endif

#ifdef DEBUG
    digitalWrite(LED_BUILTIN, HIGH);
#endif

    // Send to host
    int bytesSent = Wire.write(txBuffer, dataUnitSize);

#ifdef DEBUG
    digitalWrite(LED_BUILTIN, LOW);
#endif

    // Advance cursor by the number of sent bytes
    curOffset += bytesSent;

    // Reset cursor if overflow
    if (curOffset > (int)EEPROM.length()) {
        curOffset = 0;
    }

#ifdef DEBUG
    Serial.print("*** Sent ");
    Serial.print(bytesSent, DEC);
    Serial.println(" bytes to host ***");
#endif
}

void receiveEvent(int numBytes) {
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
        }
        else if (i == 0) {
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
    int receivedBytes = Serial.readBytes(&data[1], xModemBlockSize-1);

    return receivedBytes == (xModemBlockSize-1);
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
