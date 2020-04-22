#include <Arduino.h>
#include <ddc.h>
#include <pins.h>
#include <Wire128.h>

void setupDisplay() {
    // Use hotplugPin for hotplug advertisement
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

void setupHost() {
    Wire.begin();

    // Disable SDA/SCL pull-up
    // E-DDC specifies SCL and SDA must be pulled-up to a +5v reference using
    // resistors between 1.5K and 2.2K ohm
    digitalWrite(SDA, HIGH); // Use SDA internal pull-up resistor for dynamic host/display mode
    digitalWrite(SCL, LOW);
};