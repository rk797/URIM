#include "ReportDescParser.h"

void ReportDescParser::Parse(const uint16_t len, const uint8_t* pbuf, const uint16_t& off) {
    uint16_t actualLen = len; 

    if (offset + actualLen > bufferSize) {
        actualLen = bufferSize - offset;
    }
    memcpy(buffer + offset, pbuf, actualLen);
    offset += actualLen;
}