#include <Arduino.h>
#include <commands.h>
#include <ddc.h>
#include <Wire128.h>

// Enable debugging
//#define DEBUG


// Constants
const int hotPlugDetectPin = 7;


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

    Wire.onReceive(receiveDdcCommand);
    Wire.onRequest(sendDdcData);

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
        if (!processCommand(cmd)) {
            Serial.println("**** Failed to execute command ****");
        }
    }
}