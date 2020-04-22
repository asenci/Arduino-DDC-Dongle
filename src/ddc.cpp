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
#ifdef DEBUG
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print("*** Received read request, sending ");
    Serial.print(ddcDataUnitSize, DEC);
    Serial.print(" bytes from offset 0x");
    Serial.print(curOffset, HEX);
    Serial.println(" ***");

    for (int i = 0; i < ddcDataUnitSize; i++) {
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

    byte buffer[ddcDataUnitSize];
    EEPROM.get(curOffset, buffer);

    // Send to host
    int bytesSent = Wire.write(buffer, ddcDataUnitSize);

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

