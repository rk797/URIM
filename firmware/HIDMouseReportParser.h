#if !defined(__HIDMOUSERPTPARSER_H__)
#define __HIDMOUSERPTPARSER_H__

#include <usbhid.h>
#include "Structures.h"

#define MOUSE_LEFT      1
#define MOUSE_RIGHT     2
#define MOUSE_MIDDLE    4
#define MOUSE_BUTTON4   8
#define MOUSE_BUTTON5   16

/* If no other implementation is provided, the weak implementation will be used. */
void onButtonUp(uint16_t buttonId) __attribute__((weak));
void onButtonDown(uint16_t buttonId) __attribute__((weak));
void onTiltPress(int8_t tiltValue) __attribute__((weak));
void onMouseMove(int16_t xMovement, int16_t yMovement, int8_t scrollValue) __attribute__((weak));

extern HIDReportInfo reportInfo;

class HIDMouseReportParser : public HIDReportParser {
    uint8_t previousButtonsState;
    uint8_t protocol;
public:
    HIDMouseReportParser(void*);
    virtual void Parse(USBHID* hid, bool is_rpt_id, uint8_t len, uint8_t* buf);
    HIDReportDescriptor SetReportDescriptor(USBHID* hid);
    uint8_t GetProtocol() const { return protocol; }
private:
    bool descAcquired;
    void ParseBufIndex(const uint8_t* descriptor, uint16_t size);
    void ProgressDescriptorItem(const uint8_t* descriptor, uint16_t& index, uint8_t& item, uint8_t& itemSize, uint32_t& data);
    bool RetrieveReportDescriptor(USBHID* hid, uint8_t* buffer, size_t bufferSize, HIDReportDescriptor& reportDescriptor, uint16_t wIndex);
    bool RetrieveInterfaceDescriptor(USBHID* hid);
    bool SetProtocol(USBHID* hid, uint8_t protocol, uint8_t iface);
    int indexOf(char* array, uint8_t len, char element);
    
};

#endif //__HIDMOUSERPTPARSER_H__
