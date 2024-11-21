#include "RKHIDUniversal.h"

uint8_t RKHIDUniversal::Poll()
{
    uint8_t rcode = 0;
    if (!bPollEnable)
        return 0;
    if ((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) {
        qNextPollTime = (uint32_t)millis() + 1;
        // millis + poll interval
        // hard code to 1ms for now

        uint8_t buf[constBuffLen];

        for (uint8_t i = 0; i < bNumIface; i++) {

            if (hidInterfaces[i].bmProtocol != USB_HID_PROTOCOL_MOUSE)
                continue;
            uint8_t index = hidInterfaces[i].epIndex[epInterruptInIndex];
            uint16_t read = (uint16_t)epInfo[index].maxPktSize;

            ZeroMemory(constBuffLen, buf);

            // https://github.com/felis/USB_Host_Shield_2.0/blob/ef5b046d5058b561ddaee69bc41b7e3f672aa9ee/hidcomposite.cpp#L370
            rcode = pUsb->inTransfer(bAddress, epInfo[index].epAddr, &read, buf);

            if (rcode) {
                if (rcode != hrNAK)
                    USBTRACE3("(hiduniversal.h) Poll:", rcode, 0x81);
                return rcode;
            }

            if (read > constBuffLen)
                read = constBuffLen;

            // TODO: handle read == 0 ? continue like HIDComposite,
            //       return early like in the error case above?
            //       Either way passing an empty buffer to the functions below
            //       probably makes no sense?

#if 0
            Notify(PSTR("\r\nBuf: "), 0x80);

            for (uint8_t i = 0; i < read; i++) {
                D_PrintHex<uint8_t >(buf[i], 0x80);
                Notify(PSTR(" "), 0x80);
            }

            Notify(PSTR("\r\n"), 0x80);
#endif
            ParseHIDData(this, bHasReportId, (uint8_t)read, buf);

            HIDReportParser* prs = GetReportParser(((bHasReportId) ? *buf : 0));

            if (prs)
                prs->Parse(this, bHasReportId, (uint8_t)read, buf);
        }
    }
    return rcode;
}