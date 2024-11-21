#include "HIDMouseReportParser.h"
#include "CustomMouse.h"
#include "RKHIDUniversal.h"
#include "RawHID.h"
#include <avr/pgmspace.h>
//#include "CEEPROM.h"
#include <util/crc16.h>
#include <UsbCore.h>

#define BUFFER_PING_BYTE		0       // byte ping
#define BUFFER_XDELTA           1       // 2-byte xdelta
#define BUFFER_YDELTA           3       // 2-byte ydelta

#define BUFFER_TX_BUTTONS       6       // byte button

#define BUFFER_CMD_CONTEXT      5       // byte command
#define BUFFER_CMD_DATA_LOW     7       // low byte
#define BUFFER_CMD_DATA_HIGH    8       // high byte

#define BOOT_START			    0x7000
#define BOOT_SIZE			    0x1000  // 4kb

#define KNOWN_CRC			    0xE9FE



/* USR CMD */
#define CMD_CLICK               0x01
#define CMD_DPI                 0x02

USB Usb;
RKHIDUniversal Hid(&Usb);
HIDMouseReportParser Mou(nullptr);

uint8_t rawhidData[RAWHID_SIZE];
uint8_t _prevButtonState = 0;

bool bClientConnected = false;
bool bHasBootloader = false;

int iCurrentDPI = 800;

//bool BL_VerifySignature() {
//
//    /* Known byte sequence */
//    const uint8_t sig[] = { 0x0C, 0x94, 0x34, 0xFF, 0x0C, 0x94, 0x45, 0xFF };
//    const uint16_t sigAddr = 0x7E00; // address of the sig
//    const uint8_t sigLen = sizeof(sig);
//
//    for (uint8_t i = 0; i < sigLen; i++) {
//        uint8_t data = pgm_read_byte_near(sigAddr + i);
//        if (data != sig[i]) {
//            return false;
//        }
//    }
//    return true;
//}

bool BL_VerifyCRC() {
    uint16_t crc = 0xFFFF;
    for (uint32_t addr = BOOT_START; addr < BOOT_START + BOOT_SIZE; addr++) {
        uint8_t data = pgm_read_byte_near(addr);
        crc = _crc16_update(crc, data);
    }
    return crc == KNOWN_CRC;
}

#pragma region IOCTL
/**
* Update the button state on button change
*/
void IOCTL_TransmitVendorDefinedBtnState()
{
    if (Mouse.getButtons() != _prevButtonState)
    {
        rawhidData[BUFFER_TX_BUTTONS] = Mouse.getButtons();
        _prevButtonState = Mouse.getButtons();
        RawHID.write(rawhidData, sizeof(rawhidData));
    }
}

void ICTL_ProcessUsrCmd()
{
    switch (rawhidData[BUFFER_CMD_CONTEXT])
    {
        // TODO: Change click behavior to a USR CMD instead of hard passing values
        // TODO: Implement hardware psilent
    case CMD_CLICK:
        Mouse.click();
        break;
    case CMD_DPI:

        uint8_t dpiLowByte = rawhidData[BUFFER_CMD_DATA_LOW];
        uint8_t dpiHighByte = rawhidData[BUFFER_CMD_DATA_HIGH];

        int ReceivedDPI = (dpiHighByte << 8) | dpiLowByte;
        iCurrentDPI = Clamp(ReceivedDPI, 100, 3200); // Clamp to avoid invalid values

        //CEEPROM::WriteMemory<int>(0x0, iCurrentDPI);

        break;
    default:
        break;
    }
}

bool IOCTL_CheckPing()
{
    const static uint8_t pingCode = 0xf9;
    if (rawhidData[BUFFER_PING_BYTE] == pingCode) {
        RawHID.write(rawhidData, sizeof(rawhidData));
        return true;
    }
    else return false;
}
#pragma endregion


#pragma region REGION_SERIAL
void SERIAL_TransmitVendorDefinedBtnState() {
    if (Mouse.getButtons() != _prevButtonState)
    {
        String cmd = "BTX:" + String(Mouse.getButtons()) + "\n";
        Serial1.print(cmd);
        _prevButtonState = Mouse.getButtons();
    }
}
#pragma endregion



void FlushRawHIDBuffer()
{
    RawHID.enable();
    /* reset the user cmd after loop body is done executing */
    rawhidData[BUFFER_CMD_CONTEXT] = 0;
    rawhidData[BUFFER_TX_BUTTONS] = 0;
}

void setup()
{

    Mouse.begin();
    if (Usb.Init() == -1)
        delay(200);
    if (!Hid.SetReportParser(0, &Mou))
        ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);

    // THIS REQUIRES BOOTLOADER
    //if (BL_VerifyCRC())
    //{
    Serial1.begin(115200);
    bHasBootloader = true;
    //}

    /* Load DPI */
    /*CEEPROM::ReadMemory<int>(0x0, iCurrentDPI);
    if (iCurrentDPI == 0 || iCurrentDPI == 0xFF || iCurrentDPI == -1)
    {
        iCurrentDPI = 800;
        CEEPROM::WriteMemory<int>(0x0, iCurrentDPI);
    }*/

    RawHID.begin(rawhidData, sizeof(rawhidData));
}

void loop()
{
    Usb.Task();

    /* Serial Section */
    if (bHasBootloader)
    {
        if (Serial1)
        {
            SERIAL_TransmitVendorDefinedBtnState();

            if (Serial1.available() > 0) {

                String command = Serial1.readStringUntil('\n');
                ParseCommand(command);
                return;
            }
        }
    }

    /* HID Section */
    if (bClientConnected)
    {
        IOCTL_TransmitVendorDefinedBtnState();
    }

    if (!RawHID.available())
        return;

    ICTL_ProcessUsrCmd();

    if (IOCTL_CheckPing())
    {
        bClientConnected = true;
    }
    else if (bClientConnected)
    {

        int xDelta = (int8_t)rawhidData[BUFFER_XDELTA];
        int yDelta = (int8_t)rawhidData[BUFFER_YDELTA];

        // check to make sure we are not sending a null packet
        if (xDelta == 0 && yDelta == 0) return;

        Mouse.move(xDelta, yDelta, 0);

    }
    FlushRawHIDBuffer();
}

int Clamp(int value, int minVal, int maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

void ParseCommand(String command)
{
    command.trim();
    if (command.startsWith("km.move("))
    {
        int firstComma = command.indexOf(',');
        int secondComma = command.indexOf(',', firstComma + 1);
        int closeParen = command.indexOf(')', secondComma + 1);

        int xValue = command.substring(8, firstComma).toInt();
        int yValue = command.substring(firstComma + 1, secondComma).toInt();

        if (xValue == 0 && yValue == 0) return;
        Mouse.move(xValue, yValue, 0);
    }
    else if (command.startsWith("km.click("))
    {
        int sBTN = command.substring(9, command.indexOf(')')).toInt();
        int button;
        switch (sBTN) {
        case 0:
            button = MOUSE_LEFT;
            break;
        case 1:
            button = MOUSE_RIGHT;
            break;
        case 2:
            button = MOUSE_MIDDLE;
            break;
        case 3:
            button = MOUSE_BUTTON4;
            break;
        case 4:
            button = MOUSE_BUTTON5;
            break;
        default:
            button = MOUSE_LEFT;
            break;
        }
        Mouse.click();
    }
    else if (command.startsWith("km.version()"))
    {
        Serial1.println("kmbox: 69.420\n");
    }
    else if (command.startsWith("km.silent("))
    {
        int firstComma = command.indexOf(',');
        int secondComma = command.indexOf(',', firstComma + 1);
        int closeParen = command.indexOf(')', secondComma + 1);

        int xValue = command.substring(10, firstComma).toInt();
        int yValue = command.substring(firstComma + 1, secondComma).toInt();

        if (xValue == 0 && yValue == 0) return;

        Mouse.MoveVector2D(xValue, yValue);
        Mouse.click();
        Mouse.MoveVector2D(-xValue, -yValue);
    }
}


void onButtonDown(uint16_t buttonId)
{
    Mouse.press(buttonId);
}

void onButtonUp(uint16_t buttonId)
{
    Mouse.release(buttonId);
}

void onTiltPress(int8_t tiltValue)
{
    Serial.println(tiltValue);
}

void onMouseMove(int16_t xMovement, int16_t yMovement, int8_t scrollValue)
{
    float scaleFactor = (float)iCurrentDPI / 800.0;

    int x = round(xMovement * scaleFactor);
    int y = round(yMovement * scaleFactor);

    Mouse.move(x, y, scrollValue);
}
