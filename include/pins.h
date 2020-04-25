#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

const int hotPlugPin = 8;
const int vccDetectPin = 7;
const int vccEnablePin = 6;

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif // LED_BUILTIN

#endif //PINS_H
