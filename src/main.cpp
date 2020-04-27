#include <Arduino.h>
#include <ddc.h>
#include <debug.h>
#include <pins.h>
#include <Wire.h>
#include <xmodem.h>


// cppcheck-suppress unusedFunction
void setup() {
    // Initialise software serial for debugging
    SerialDebug.begin(115200);
    SerialDebug.println(F("***** Booting up *****"));

    // Use vccDetectPin for host VCC detection
    pinMode(VCC_DETECT, INPUT);
    digitalWrite(VCC_DETECT, LOW);

    // Use vccEnablePin for sink power supply
    // Host must provide a +5v power source before the sink can advertise hot plug
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, LOW);

    // Enter host mode if host Vcc is not detected
    if (digitalRead(VCC_DETECT) == LOW) {
        SerialDebug.println(F("**** Host power not detected, entering host mode ****"));

        // Use hotPlugDetectPin for hotplug detection
        // Sink must pull-up to a +5v reference to signal presence to the host
        pinMode(HOT_PLUG, INPUT);
        digitalWrite(HOT_PLUG, LOW);

        // Register as i2c master
        Wire.begin();

        // Register hot plug detection interrupt
        attachInterrupt(digitalPinToInterrupt(HOT_PLUG), ddcDumpEdid, RISING);

        // Present power to sink
        SerialDebug.println(F("**** Presenting power to sink ****"));
        digitalWrite(VCC_ENABLE, HIGH);

        // DDC specifies sink must be ready to reply to host within 20ms
        delay(20);

    } else {
        SerialDebug.println(F("**** Host power detected, entering sink mode ****"));

        // Use hotPlugPin for hot plug advertisement
        // Sink must pull-up to a +5v reference to signal presence to the host
        pinMode(HOT_PLUG, OUTPUT);
        digitalWrite(HOT_PLUG, LOW);

        // Register as i2c slave
        Wire.begin(ddcAllAddresses);
        TWAMR = ddcAllAddressesMask << 1u;

        // Disable SDA/SCL pull-up
        // DDC specifies sink must pull-up SCL to a +5v reference using a 47k ohm resistor
        digitalWrite(SDA, LOW);
        digitalWrite(SCL, LOW);

        Wire.onReceive(receiveDdcCommand);
        Wire.onRequest(receiveDdcReadRequest);

        SerialDebug.println(F("**** Advertising hot plug ****"));
        digitalWrite(HOT_PLUG, HIGH);
    }

    // Initialise hardware serial for xModem transfer
    Serial.begin(9600);
}


// cppcheck-suppress unusedFunction
void loop() {
    xModemMainLoop();
}