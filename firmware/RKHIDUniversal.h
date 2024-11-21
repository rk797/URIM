#include "hiduniversal.h"

class RKHIDUniversal : public HIDUniversal {
public:
    RKHIDUniversal(USB* pUsb) : HIDUniversal(pUsb) {}
    uint8_t Poll() override;

};
