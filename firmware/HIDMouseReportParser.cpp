#include "HIDMouseReportParser.h"
#include "ReportDescParser.h"
#include <Arduino.h>
#include "CustomMouse.h"

#define BOOT_PTRCL          0x01
#define REPORT_PTRCL        0x00

/* Descriptor ID's */
#define USAGE_PAGE          0x05
#define USAGE 			    0x09
#define INPUT               0x81
#define REPORT_SIZE			0x75
#define REPORT_COUNT		0x95
#define LOGICAL_MINIMUM 	0x16
#define LOGICAL_MAXIMUM 	0x26
#define COLLECTION          0xA1
#define REPORT_ID           0x85

HIDMouseReportParser::HIDMouseReportParser(void*) : previousButtonsState(0), descAcquired(false) {}

HIDReportInfo reportInfo;

bool HIDMouseReportParser::SetProtocol(USBHID* hid, uint8_t protocol, uint8_t iface) {
    return (hid->SetProtocol(iface, protocol) == 1);
}

void HIDMouseReportParser::Parse(USBHID* hid, bool is_rpt_id, uint8_t len, uint8_t* buf) {

    if (!descAcquired) {
        HIDReportDescriptor reportDescriptor = SetReportDescriptor(hid);

        if (reportDescriptor.size > 0) 
        {
            // Changing the descriptor after the device has initialized will not work.
            descAcquired = true;
        }
        return;
    }

    //bit shift left e.g., 1 -> 2 -> 4 -> 8 -> 16
    for (uint16_t buttonId = 1; buttonId <= 16; buttonId <<= 1) 
    {
        if (buf[reportInfo.buttonsIndex] & buttonId) 
        {
            if (!(previousButtonsState & buttonId)) 
            {
                onButtonDown(buttonId);
            }
        }
        else 
        {
            if (previousButtonsState & buttonId) 
            {
                onButtonUp(buttonId);
            }
        }
    }
    
    
    int16_t xMovement, yMovement;
    int8_t scrollValue = buf[reportInfo.wheelIndex];

    /* 
    debug
    memcpy(&xMovement, &buf[2], 2);
    memcpy(&yMovement, &buf[4], 2);
    */

    if (reportInfo.xySize == 16) {
        previousButtonsState = buf[reportInfo.buttonsIndex] | (buf[1] << 8);

        //For the gpro wireless mouse
        //previousButtonsState = buf[1];

        memcpy(&xMovement, &buf[reportInfo.xIndex], 2);
        memcpy(&yMovement, &buf[reportInfo.yIndex], 2);
    }
    else {
        //previousButtonsState = buf[0];

        xMovement = buf[reportInfo.xIndex];
        yMovement = buf[reportInfo.yIndex];
    }

    /* Debug */
    /*if (len > 0) {
        Serial.print("Time: ");
        Serial.print(millis());
        Serial.print("ms Buffer: ");
        for (uint8_t i = 0; i < len; i++) {
            Serial.print(buf[i], HEX);
            Serial.print(" ");
        }
        Serial.print("PB: ");
        Serial.print(previousButtonsState, BIN);
        Serial.println();
    }*/

    if (xMovement != 0 || yMovement != 0 || scrollValue != 0)
    {
        onMouseMove(xMovement, yMovement, scrollValue);
    }
}

bool HIDMouseReportParser::RetrieveReportDescriptor(USBHID* hid, uint8_t* buffer, size_t bufferSize, HIDReportDescriptor& reportDescriptor, uint16_t wIndex) 
{
    ReportDescParser parser(buffer, bufferSize);

    uint16_t rcode = hid->GetReportDescr(wIndex, &parser);
    if (rcode == 0) 
    {
        //Serial.println("Mouse Report Descriptor:");
        for (uint16_t i = 0; i < parser.GetLength(); i++) 
        {
            /*Serial.print(buffer[i], HEX);
            Serial.print(" ");*/
            reportDescriptor.descriptor[i] = buffer[i];
        }
        reportDescriptor.size = parser.GetLength();
        //Serial.println();
        return true;
    }
    else 
    {
        //Serial.println("Failed to retrieve report descriptor.");
        reportDescriptor.size = 0;
        return false;
    }
}

bool HIDMouseReportParser::RetrieveInterfaceDescriptor(USBHID* hid) 
{
    uint8_t protocol;
    uint8_t iface = 0;

    uint8_t rcode = hid->GetProtocol(iface, &protocol);
}

HIDReportDescriptor HIDMouseReportParser::SetReportDescriptor(USBHID* hid)
{
    uint8_t buf[256];
    HIDReportDescriptor reportDescriptor;
    uint16_t wIndex = 0;

    /**
    * TODO: Implement a better way to handle multiple interfaces.
    *
    */
    const uint8_t maxInterfaces = 3;

    bool validInterfaceFound = false;

    while (wIndex < maxInterfaces) {
        if (!RetrieveReportDescriptor(hid, buf, sizeof(buf), reportDescriptor, wIndex)) {
            return;
        }

        ParseBufIndex(reportDescriptor.descriptor, reportDescriptor.size);

        if (reportInfo.xIndex != 0 && reportInfo.yIndex != 0 && reportInfo.wheelIndex != 0) {
            validInterfaceFound = true;
            break;
        }

        wIndex++;
    }

    if (!validInterfaceFound) {
        //Serial.println(F("Error: No valid HID interface found for the mouse."));
        return;
    }

    
    Serial1.println();
    Serial1.print("X Index: ");
    Serial1.println(reportInfo.xIndex);
    Serial1.print("Y Index: ");
    Serial1.println(reportInfo.yIndex);
    Serial1.print("Wheel Index: ");
    Serial1.println(reportInfo.wheelIndex);
    Serial1.print("XY Size: ");
    Serial1.println(reportInfo.xySize);
    Serial1.print("Button Index: ");
    Serial1.println(reportInfo.buttonsIndex);
    Serial1.print("Button Size: ");
    Serial1.println(reportInfo.buttonsSize);
    Serial1.println();
    

    return reportDescriptor;  // Return the valid report descriptor
}
void HIDMouseReportParser::ProgressDescriptorItem(const uint8_t* descriptor, uint16_t& index, uint8_t& item, uint8_t& itemSize, uint32_t& data) {
    item = descriptor[index++];
    itemSize = item & 0x03;
    if (itemSize == 3) itemSize = 4;
    data = 0;
    if (itemSize > 0) {
        memcpy(&data, &descriptor[index], itemSize);
        index += itemSize;
    }
}

int HIDMouseReportParser::indexOf(char* array, uint8_t len, char element) {
    for (int i = 0; i < len; i++) {
        if (array[i] == element) {
            return i;
        }
    }
    return -1;
}

void HIDMouseReportParser::ParseBufIndex(const uint8_t* descriptor, uint16_t size) {
    uint16_t bitOffset = 0;
    uint8_t reportSize = 0;
    uint8_t reportCount = 0;
    uint16_t index = 0;
   

    bool foundX = false;
    bool foundY = false;
    bool foundWheel = false;
    bool foundButtonsStage1 = false;
    bool foundButtonsStage2 = false;
    bool isXYSet = false;
    bool isWheelSet = false;

    uint8_t item;
    uint8_t itemSize;
    uint32_t data = 0;

    uint8_t o_len = 3;
    char order[o_len];
    
    uint8_t _index = 0;

    while (index < size) 
    {
        
        reportCount = 0;
        reportSize = 0;

        ProgressDescriptorItem(descriptor, index, item, itemSize, data);

        #pragma region Button Stage 1
        if (item == COLLECTION && data == 0x01 && !foundButtonsStage1)
        {
            ProgressDescriptorItem(descriptor, index, item, itemSize, data);
            if (item == REPORT_ID)
            {
                // need to shift everything to the right by 1 byte to make room for the report ID :)
                // could probably pass in the boolean flag here from caller instead of creating our own custom logic
                reportInfo.reportID = data;
                bitOffset += 8;
                foundButtonsStage1 = true;
            }
            else if (item == USAGE && data == 0x01) // Usage (Pointer)
            {
                foundButtonsStage1 = true;
            }
            else
            {
                continue;
            }
        }
        #pragma endregion

        #pragma region Button Stage 2
        if ((item == COLLECTION && data == 0x00) && !foundButtonsStage2 && foundButtonsStage1) {
            while (item != INPUT) {
                ProgressDescriptorItem(descriptor, index, item, itemSize, data);
                switch (item) {
                case REPORT_SIZE:
                    reportSize = data;
                    break;
                case REPORT_COUNT:
                    reportCount = data;
                    break;
                case USAGE_PAGE:
                    if (data == 0x09)
                    {
                        foundButtonsStage2 = true;
                    }
                default:
                    break;
                }
            }
            if (foundButtonsStage2) {
                reportInfo.buttonsSize = reportSize * reportCount;
                reportInfo.buttonsIndex = bitOffset / 8;
                bitOffset += reportSize * reportCount;
                continue;
            }
        }
        #pragma endregion

        #pragma region XY Parsing
        while (item != INPUT && foundButtonsStage2) {
            /* Processing */
            switch (item) {
            case REPORT_SIZE:
                reportSize = data;
                break;
            case REPORT_COUNT:
                reportCount = data;
                break;
            case USAGE:
                if (data == 0x30 && !foundX)
                {
                    order[_index++] = 'X';
                    foundX = true;
                }
                else if (data == 0x31 && !foundY)
                {
                    order[_index++] = 'Y';
                    foundY = true;
                }
                else if (data == 0x38 && !foundWheel)
                {
                    order[_index++] = 'W';
                    foundWheel = true;
                }
                break;
            default:
                break;
            }
            ProgressDescriptorItem(descriptor, index, item, itemSize, data);
        }

        if (foundX && foundY && !isXYSet) {

            /* handle 16 bit */
            if (reportSize == 16)
            {
                reportInfo.xIndex = bitOffset / 8;
                reportInfo.yIndex = bitOffset / 8 + 2;
                reportInfo.xySize = reportSize;
            }
            /* handle 8 bit */
            else
            {
                reportInfo.xIndex = bitOffset / 8 + indexOf(order, o_len, 'X');
                reportInfo.yIndex = bitOffset / 8 + indexOf(order, o_len, 'Y');
                reportInfo.xySize = reportSize;

                /* handle the wheel incase it's in the same input element */

                if (foundWheel)
                {
                    reportInfo.wheelIndex = bitOffset / 8 + indexOf(order, o_len, 'W');
                    reportInfo.wheelSize = reportSize;
                }
            }

            isXYSet = true;
        }
        else if (foundWheel && !isWheelSet) {
            reportInfo.wheelIndex = bitOffset / 8;
            reportInfo.wheelSize = reportSize;
            isWheelSet = true;
        }
        #pragma endregion

        

        if (foundWheel && foundX && foundY && foundButtonsStage2) break;

        // Ninjitsu developers have not specified a report size in their padding
        // For cases like this, we will assume a size of 1 bit
        // This is a workaround method to handle the padding issue
        if (reportCount > 0 && reportSize == 0 && foundButtonsStage2 && !foundX) reportSize = 1;
        bitOffset += reportSize * reportCount;
        
        
        /*
        Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)	81 02 
        Report Count (3)	                        95 03 
        Input (Cnst,Ary,Abs)	                    81 01 
        */
    }
}