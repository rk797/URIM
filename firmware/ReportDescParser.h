#ifndef REPORTDESCPARSER_H
#define REPORTDESCPARSER_H

#include <usbhid.h>

class ReportDescParser : public USBReadParser {
    uint8_t* buffer;
    uint16_t bufferSize;
    uint16_t offset;

public:
    ReportDescParser(uint8_t* buf, uint16_t size) : buffer(buf), bufferSize(size), offset(0) {}
    void Parse(const uint16_t len, const uint8_t* pbuf, const uint16_t& off) override;
    uint16_t GetLength() const { return offset; }
};

#endif // REPORTDESCPARSER_H
