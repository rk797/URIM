#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>

struct HIDReportDescriptor {
    uint8_t descriptor[256] PROGMEM;
    uint16_t size;
};

struct HIDReportInfo {
    uint8_t xIndex;
    uint8_t xySize;
    uint8_t yIndex;
    uint8_t wheelIndex;
    uint8_t wheelSize;
    uint8_t buttonsSize;
    uint8_t buttonsIndex;
    uint8_t reportID;
};

struct ButtonState {
    uint8_t buttonId;
    unsigned long pressTime;
    bool isPressed;
};

#endif // STRUCTURES_H
