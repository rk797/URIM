#ifndef RK_MOUSE_H
#define RK_MOUSE_H

#include <iostream>
#include <cstring>
#include <hidapi.h>
#include <future>
#include <thread>
#include <chrono>


#define MOUSE_LEFT          1
#define MOUSE_RIGHT         2
#define MOUSE_MIDDLE        4
#define MOUSE_ALL           (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)
#define PINGCODE            0xf9
#define BTN_REQUEST_CODE    0xF2
#define BUFFER_TX_BUTTONS   6

class rkMouse {
public:
    rkMouse(hid_device* dev);
    static rkMouse getMouse(int pingCode = PINGCODE);
    static rkMouse getMouse(int vid, int pid);

    void click(int button = MOUSE_LEFT);
    void press(int button = MOUSE_LEFT);
    void release(int button = MOUSE_LEFT);
    bool isPressed(int button = MOUSE_LEFT);
    void move(int x, int y);

    bool GetArduinoKeyState(uint8_t vkey);

    void startAsyncPolling();
    void stopAsyncPolling();

private:
    void buttons(int buttons);
    void sendRawReport(unsigned char* reportData);
    unsigned char* makeReport(int x, int y);

    int limitXY(int xy);
    uint8_t lowByte(int x);
    uint8_t highByte(int x);
    static bool checkPing(hid_device* dev, int pingCode);

    int req_buttons(hid_device* dev, uint8_t* buffer);
    void pollButtons();

    hid_device* dev;
    uint8_t buttonsMask;
    uint8_t rawhidbuffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    std::atomic<int> last_button_state{ 0 };
    std::atomic<bool> stop_thread{ false };
    std::future<void> polling_future;

};

struct DeviceInfo {
    int vendorId;
    int productId;
    std::wstring manufacturerString;
};

DeviceInfo getDeviceInfo(const wchar_t* serialNumber);

#endif // RK_MOUSE_H
