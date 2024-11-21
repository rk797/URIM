#include "rkMouseEther.hpp"

/**
* DISCLAIMER: RKMOUSEETHER HAS BEEN DEPRECATED AND IS NO LONGER SUPPORTED.
*/
int main() {
    const char* addr = "xxxxxxxxx";
    rkMouseEther rkMouse(addr);

    //std::thread hookThread(&rkMouseEther::SetMouseHook, &rkMouse);
    //hookThread.detach();

    if (rkMouse.Initialize()) 
    {
        rkMouse.MainLoop(); 
    }

    rkMouse.CloseSocket();
    WSACleanup();
    return 0;
}
