#include <EEPROM.h>
#include "src/Wire128/src/Wire.h"

// Enable debugging
// #define DEBUG

// Constants
const int dataUnitSize = 128;

const int ddcPriAddress = 0xA0 >> 1;
const int ddcSecAddress = 0xA4 >> 1;
const int ddcSpAddress = 0x60 >> 1;

const int hotplugPin = 7;

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
  pinMode(hotplugPin, OUTPUT);
  digitalWrite(hotplugPin, LOW);

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

  digitalWrite(hotplugPin, HIGH);
}


void loop() {
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
  if (curOffset > EEPROM.length()) {
    curOffset = 0;
  }

  #ifdef DEBUG
  Serial.print("*** Sent ");
  Serial.print(bytesSent, DEC);
  Serial.println(" bytes to host ***");
  #endif
}
