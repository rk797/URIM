#include "rkMouseApi.h"
/**
* RK Mouse randomizes the move(0, 0) on a hardware level to prevent detection.
* So it's fine to call move(0, 0) every time you want to change the buttons state.
*/
rkMouse::rkMouse(hid_device* dev) : dev(dev), buttonsMask(0) {
    // Nothing for now
}

DeviceInfo getDeviceInfo(const wchar_t* serialNumber) {
    hid_device* dev = nullptr;
    struct hid_device_info* devices = hid_enumerate(0, 0);
    struct hid_device_info* currentDevice = devices;

    DeviceInfo result = { 0, 0, L"" };

    while (currentDevice) {
        if (wcscmp(currentDevice->serial_number, serialNumber) == 0) {
            result.vendorId = currentDevice->vendor_id;
            result.productId = currentDevice->product_id;
            result.manufacturerString = currentDevice->manufacturer_string;
            break;
        }
        currentDevice = currentDevice->next;
    }

    hid_free_enumeration(devices);

    return result;
}

rkMouse rkMouse::getMouse(int pingCode) {
    hid_device* dev = nullptr;
    struct hid_device_info* devices = hid_enumerate(0, 0);
    struct hid_device_info* currentDevice = devices;

    while (currentDevice) {
        if (currentDevice->release_number == 5666 || currentDevice->vendor_id == 1133) {
            try {
                //std::cout << currentDevice->path << std::endl;
                dev = hid_open_path(currentDevice->path);
                if (dev && checkPing(dev, pingCode)) {
                    hid_free_enumeration(devices);
                    return rkMouse(dev);
                }
                else {
                    hid_close(dev);
                    dev = nullptr;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[-] Error initializing device: " << e.what() << std::endl;
            }
        }
        currentDevice = currentDevice->next;
    }

    hid_free_enumeration(devices);
    throw std::runtime_error("[-] Device not found");
}

rkMouse rkMouse::getMouse(int vid, int pid) {
    hid_device* dev = nullptr;
    struct hid_device_info* devices = hid_enumerate(vid, pid);
    struct hid_device_info* currentDevice = devices;

    while (currentDevice) {
        if (currentDevice->vendor_id == vid && currentDevice->product_id == pid) {
            try {
                std::cout << currentDevice->path << std::endl;
                dev = hid_open_path(currentDevice->path);
                std::cout << currentDevice << std::endl;
                if (dev) {
                    hid_free_enumeration(devices);
                    return rkMouse(dev);
                }
                else {
                    hid_close(dev);
                    dev = nullptr;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[-] Error initializing device: " << e.what() << std::endl;
            }
        }
        currentDevice = currentDevice->next;
    }

    hid_free_enumeration(devices);
    throw std::runtime_error("[-] Device not found");
}

void rkMouse::buttons(int buttons) {
    if (buttons != buttonsMask) {
        buttonsMask = buttons;
        move(0, 0);
    }
}

void rkMouse::click(int button) {
    buttonsMask = button;
    move(0, 0);
    buttonsMask = 0;
    move(0, 0);
}

void rkMouse::press(int button) {
    buttons(buttonsMask | button);
}

void rkMouse::release(int button) {
    buttons(buttonsMask & ~button);
}

bool rkMouse::isPressed(int button) {
    return static_cast<bool>(button & buttonsMask);
}

void rkMouse::move(int x, int y) {
    int limitedX = limitXY(x);
    int limitedY = limitXY(y);
    sendRawReport(makeReport(limitedX, limitedY));
}

unsigned char* rkMouse::makeReport(int x, int y) {
    unsigned char* reportData = new unsigned char[6] 
    {
        0x01,                // Report ID: 0
        (uint8_t) buttonsMask,
        lowByte(x),
        highByte(x),
        lowByte(y), 
        highByte(y)
    };
    return reportData;
}

void rkMouse::sendRawReport(unsigned char* reportData) {
    hid_write(dev, reportData, 6);
    delete[] reportData;
}

bool rkMouse::checkPing(hid_device* dev, int pingCode) {
    unsigned char pingData[2] = { 0, static_cast<unsigned char>(pingCode) };
    hid_write(dev, pingData, 2);

    unsigned char resp[1];
    int bytesRead = hid_read_timeout(dev, resp, 1, 100);
    return bytesRead > 0 && resp[0] == static_cast<unsigned char>(pingCode);
}

void rkMouse::startAsyncPolling() {
    polling_future = std::async(std::launch::async, &rkMouse::pollButtons, this);
}

void rkMouse::stopAsyncPolling() {
    stop_thread = true;
    if (polling_future.valid()) {
        polling_future.get();
    }
}

void rkMouse::pollButtons() {
    while (!stop_thread) {
        int button_state = req_buttons(dev, rawhidbuffer);

        if (button_state >= 0) {
            last_button_state = button_state;
        }

        // small delay to avoid constant polling
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool rkMouse::GetArduinoKeyState(uint8_t btn) {
    int button_state = last_button_state;

    switch (btn) {
    case 1:  // LEFT button
        return button_state & 0x01;
    case 2:  // RIGHT button
        return button_state & 0x02;
    case 8:  // MOUSE_X4
        return button_state & 0x08;
    case 16: // MOUSE_X5
        return button_state & 0x10;
    default:
        return false;
    }
}

int rkMouse::req_buttons(hid_device* dev, uint8_t* buffer) {

    int res = hid_read_timeout(dev, buffer, 8, 10);
    std::cout << "res: " << res << std::endl;
    if (res < 0) {
        std::cerr << "[-] Error reading from device." << std::endl;
        return -1;
    }
    return buffer[BUFFER_TX_BUTTONS];
}

int rkMouse::limitXY(int xy) {
    if (xy < -32767) {
        return -32767;
    }
    else if (xy > 32767) {
        return 32767;
    }
    else {
        return xy;
    }
}

uint8_t rkMouse::lowByte(int x) {
    return x & 0xFF;
}

uint8_t rkMouse::highByte(int x) {
    return (x >> 8) & 0xFF;
}
