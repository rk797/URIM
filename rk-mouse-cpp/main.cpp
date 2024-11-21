#include "rkMouseApi.h"
#include <windows.h>
#include <thread>

 
int main() {

    //Fetch the mouse
    rkMouse mouse = rkMouse::getMouse();

    //Fetch mouse based off vid & pid
    /*rkMouse mouse2 = rkMouse::getMouse(0x1234, 0x5678);*/

    // move mouse 100 pixels in x and y direction
    mouse.move(100, 100);

    // start async polling for arduino button states
    mouse.startAsyncPolling();
    while (true) 
    {
        bool left_pressed = mouse.GetArduinoKeyState(1);
        bool right_pressed = mouse.GetArduinoKeyState(2);

        printf("Left button: %d, Right button: %d\n", left_pressed, right_pressed);
    }
    // stop async polling
    mouse.stopAsyncPolling();

    return 0;
}