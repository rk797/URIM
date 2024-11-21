#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

class rkMouseEther
{
public:
    const int MAX_VALUE = 127;
    const int SCREEN_WIDTH;
    const int SCREEN_HEIGHT;
    const int CENTER_X;
    const int CENTER_Y;
    const char* addr;

    volatile LONG g_scrollDelta = 0;
    std::atomic<int> g_lastX{ 0 };
    std::atomic<int> g_lastY{ 0 };

    SOCKET sock = NULL;

    rkMouseEther(const char* ipAddress)
        : SCREEN_WIDTH(GetSystemMetrics(SM_CXSCREEN)),
        SCREEN_HEIGHT(GetSystemMetrics(SM_CYSCREEN)),
        addr(ipAddress),
        CENTER_X(SCREEN_WIDTH / 2),
        CENTER_Y(SCREEN_HEIGHT / 2)
    {
        std::cout << "rkMouseEther instance created with address: " << addr << std::endl;
    }

    bool Initialize() {
        WSADATA wsaData;
        SOCKET s = INVALID_SOCKET;
        struct sockaddr_in server;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed.\n";
            return false;
        }

        if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            std::cerr << "Could not create socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            return false;
        }

        int flag = 1;
        if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
            std::cerr << "Could not set TCP_NODELAY: " << WSAGetLastError() << "\n";
            closesocket(s);
            WSACleanup();
            return false;
        }

        unsigned long mode = 1;
        if (ioctlsocket(s, FIONBIO, &mode) != 0) {
            std::cerr << "Error setting non-blocking mode: " << WSAGetLastError() << "\n";
            closesocket(s);
            WSACleanup();
            return false;
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(7000);
        server.sin_addr.s_addr = inet_addr(this->addr);

        if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                std::cerr << "Connect error: " << WSAGetLastError() << "\n";
                closesocket(s);
                WSACleanup();
                return false;
            }
        }

        std::cout << "Connected to Arduino!\n";
        this->sock = s;
        return true;
    }

    void CloseSocket() {
        closesocket(this->sock);
    }

    /*static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;

            if (wParam == WM_MOUSEWHEEL) {
                g_scrollDelta += GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) / WHEEL_DELTA;
            }
            
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    void SetMouseHook() {
        HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
        if (hMouseHook == NULL) {
            std::cerr << "Mouse hook failed." << std::endl;
            return;
        }

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnhookWindowsHookEx(hMouseHook);
    }*/

    void SendMouse(int x = 0, int y = 0, int wheel = 0, int btn = 0, int sidebtn = 0) {
        limitxy(x, y);
        char data[5];
        data[0] = static_cast<char>(x);
        data[1] = static_cast<char>(y);
        data[2] = static_cast<char>(wheel);
        data[3] = static_cast<char>(btn);
        data[4] = static_cast<char>(sidebtn);

        std::cout << "Sending mouse data: x = " << x << ", y = " << y << ", wheel = " << wheel << ", btn = " << btn << ", sidebtn = " << sidebtn << std::endl;
        send(this->sock, data, sizeof(data), 0);
    }

    void limitxy(int& x, int& y) {
        x = max(min(x, MAX_VALUE), -MAX_VALUE);
        y = max(min(y, MAX_VALUE), -MAX_VALUE);
    }

    void MainLoop() {
        
        SetCursorPos(this->CENTER_X, this->CENTER_Y);
        g_lastX = this->CENTER_X;
        g_lastY = this->CENTER_Y;

        SendMouse();

        bool leftButtonDown = false;
        bool rightButtonDown = false;
        int btnByte = 0;

        while (!GetAsyncKeyState(VK_ESCAPE)) {
            POINT currentPos;
            GetCursorPos(&currentPos);

            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (!leftButtonDown) {
                    leftButtonDown = true;
                    btnByte = 1;
                }
            }
            else if (leftButtonDown) {
                leftButtonDown = false;
                btnByte = 2;
            }

            if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                if (!rightButtonDown) {
                    rightButtonDown = true;
                    btnByte = 3;
                }
            }
            else if (rightButtonDown) {
                rightButtonDown = false;
                btnByte = 4;
            }

            LONG scrollDelta = InterlockedExchange(&g_scrollDelta, 0);

            int dx = currentPos.x - this->CENTER_X;
            int dy = currentPos.y - this->CENTER_Y;

            if (dx != 0 || dy != 0) {
                SendMouse(dx, dy, scrollDelta, btnByte, 0);
                SetCursorPos(this->CENTER_X, this->CENTER_Y);
            }

            if (btnByte != 0 || scrollDelta != 0) {
                SendMouse(0, 0, scrollDelta, btnByte, 0);
                btnByte = 0;
            }
        }
    }

private:
    static rkMouseEther* GetInstance() {
        static rkMouseEther instance(GetCurrentInstanceAddress());
        return &instance;
    }

    static const char* GetCurrentInstanceAddress() {
        return rkMouseEther::GetInstance()->addr;
    }
};
