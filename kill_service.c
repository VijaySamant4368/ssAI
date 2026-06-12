#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Executes the taskkill command completely hidden in the background
    WinExec("taskkill /f /im screenshot_service.exe", SW_HIDE);
    return 0;
}