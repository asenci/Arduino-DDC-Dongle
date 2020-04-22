#include <Arduino.h>
#include <commands.h>
#include <ddc.h>
#include <pins.h>
#include <setup.h>


// Variables
bool hostMode;
int hotplugDetected = LOW;


// cppcheck-suppress unusedFunction
void setup() {
    // Use hotplugPin for hotplug detection
    // Monitor must pull-up to a +5v reference to signal presence to the host
    // requires a external pull-down resistor for stable detection
    pinMode(hotPlugDetectPin, INPUT);
    digitalWrite(hotPlugDetectPin, LOW);

    Serial.begin(9600);

#ifdef DEBUG
    // Wait for serial interface to be ready
    while (!Serial) {}

    Serial.println(F("**** Ready ****"));

    // Use built-in LED for status
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    delay(20);
    // Enter host mode if monitor detected
    if (digitalRead(hotPlugDetectPin) == HIGH) {
#ifdef DEBUG
        Serial.println(F("**** Monitor detected on hotplug pin, entering host mode ****"));
#endif
        setupHost();
        hostMode = true;
    } else {
        setupDisplay();
        hostMode = false;
    }
}


// cppcheck-suppress unusedFunction
void loop() {
    while (Serial.available()) {
        byte cmd = Serial.read();
#ifdef DEBUG
        Serial.print(F("**** Received command from UART: 0x"));
        Serial.print(cmd, HEX);
        Serial.println(F(" ****"));
#endif
        if (!processCommand(cmd)) {
#ifdef DEBUG
            Serial.println(F("**** Failed to execute command ****"));
#endif
        }
    }

#ifdef DEBUG
    if (hostMode) {
        int newState = digitalRead(hotPlugDetectPin);

        // Signal on hotPlugDetectPin raised to +5v,
        // read EDID data from monitor
        if (hotplugDetected == LOW && newState == HIGH) {
            Serial.println(F("**** Monitor detected, reading EDID data ****"));
            dumpEDID();
            Serial.println(F("**** Done ****"));
        }

        hotplugDetected = newState;
    }
#endif
}