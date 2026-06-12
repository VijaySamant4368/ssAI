#include <windows.h>
#include <stdio.h>
#include <time.h>

#define HOTKEY_SCREEN_ID 1
#define HOTKEY_WINDOW_ID 2

void CaptureRect(const char *filename, int x, int y, int w, int h)
{
    // DC == Device Context
    //{
    //   Capture pixels from the screen
    //   Convert them into BMP format
    //   Save them to a file
    //  }
    HDC hScreenDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // BIT BLock Transfer(from specific screen coordinates (x, y) to memory DC)
    BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);
    // Freeing up memory
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    BITMAP bitmap;
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    // Metadata:
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = bitmap.bmWidth;
    bih.biHeight = bitmap.bmHeight;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    // Handles BMP row alignment requirements
    DWORD dwBmpSize = ((bitmap.bmWidth * bih.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;

    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bfh.bfType = 0x4D42; // "BM"
        bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
        bfh.bfReserved1 = 0;
        bfh.bfReserved2 = 0;
        bfh.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

        DWORD dwBytesWritten = 0;
        WriteFile(hFile, (LPSTR)&bfh, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
        WriteFile(hFile, (LPSTR)&bih, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

        char *lpBits = malloc(dwBmpSize);
        GetDIBits(hScreenDC, hBitmap, 0, (UINT)bitmap.bmHeight, lpBits, (BITMAPINFO *)&bih, DIB_RGB_COLORS);
        WriteFile(hFile, lpBits, dwBmpSize, &dwBytesWritten, NULL);

        free(lpBits);
        CloseHandle(hFile);
    }

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    DeleteDC(hScreenDC);
}

void RunPythonPipeline(const char *bmpPath)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pythonw.exe summarize.py \"%s\"", bmpPath);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

UINT AskModifiers(void)
{
    UINT mods = 0;
    char c;

    printf("CTRL? (y/n): ");
    scanf(" %c", &c);
    if (c == 'y' || c == 'Y')
        mods |= MOD_CONTROL;

    printf("ALT? (y/n): ");
    scanf(" %c", &c);
    if (c == 'y' || c == 'Y')
        mods |= MOD_ALT;

    printf("SHIFT? (y/n): ");
    scanf(" %c", &c);
    if (c == 'y' || c == 'Y')
        mods |= MOD_SHIFT;

    printf("WIN? (y/n): ");
    scanf(" %c", &c);
    if (c == 'y' || c == 'Y')
        mods |= MOD_WIN;

    return mods;
}

char AskKey(void)
{
    char key;

    printf("Key (A-Z in uppercase): ");
    scanf(" %c", &key);

    return (char)toupper(key);
}

void RegisterHotkeyInteractive(
    int hotkeyId,
    const char *description,
    UINT *modsOut,
    char *keyOut)
{
    UINT mods;
    char key;

    while (1)
    {
        printf("\n%s\n", description);

        mods = AskModifiers();
        key = AskKey();

        if (RegisterHotKey(
                NULL,
                hotkeyId,
                mods,
                key))
        {
            printf("Registered successfully.\n");
            *modsOut = mods;
            *keyOut = key;
            return;
        }

        printf(
            "Hotkey already exists. Choose another "
            "(error %lu).\n\n",
            GetLastError());
    }
}

int loadConfig(int *screenMods, char *screenKey, int *windowMods, char *windowKey)
{
    FILE *f = fopen("config.txt", "r");
    if (!f)
        return 0;
    if (fscanf(f, "%u", screenMods) != 1 || fscanf(f, " %c", screenKey) != 1 || fscanf(f, "%u", windowMods) != 1 || fscanf(f, " %c", windowKey) != 1)
    {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

void saveConfig(UINT screenMods, char screenKey, UINT windowMods, char windowKey)
{
    FILE *f = fopen("config.txt", "w");
    if (!f)
        return;

    fprintf(f, "%u\n", screenMods);
    fprintf(f, "%c\n", screenKey);
    fprintf(f, "%u\n", windowMods);
    fprintf(f, "%c\n", windowKey);

    fclose(f);
    // FILE *verify = fopen("config.txt", "rb");

    // int ch;
    // while ((ch = fgetc(verify)) != EOF)
    // {
    //     printf("%02X ", (unsigned char)ch);
    // }

    // printf("\n");
    // fclose(verify);
    // 
    }

void AddToStartup(void)
{
    HKEY hKey;

    if (RegOpenKeyEx(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_SET_VALUE,
            &hKey) == ERROR_SUCCESS)
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);

        RegSetValueExA(
            hKey,
            "ScreenshotHotkeys",
            0,
            REG_SZ,
            (BYTE*)path,
            strlen(path) + 1
        );

        RegCloseKey(hKey);
    }
}


// Entry POint
// WINAPI is jyst a macro (#define WINAPI __stdcall)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
                   // !!!!!!!!!!!!!!!!!!!!!!!!!
                   ,
                   LPSTR lpCmdLine, int nCmdShow
                   // !!!!!!!!!!!!!!!!!!!!!!!!!

                   // May use these two later
)
{

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    //Remove the executable name, leaving only the directory path
    char *lastSlash = strrchr(exePath, '\\');
    if (lastSlash != NULL)
    {
        *lastSlash = '\0';
        SetCurrentDirectoryA(exePath); //Set working directory
    }

    // int isFirstRun = 1;

    UINT screenMods, windowMods;
    char screenKey, windowKey;

    
    // //I think I can also write this where isFirstRun is being updated, but it will cause unnecessary registry writes, as opposed to rewring an int
    // if (isFirstRun)     

    //If incase someone has config BEFORE starting the service even once
    AddToStartup();

    if (!loadConfig(&screenMods, &screenKey, &windowMods, &windowKey) || !RegisterHotKey(NULL, HOTKEY_SCREEN_ID, screenMods, screenKey) || !RegisterHotKey(NULL, HOTKEY_WINDOW_ID, windowMods, windowKey))
    {
        // isFirstRun = 0;
        AddToStartup();
        AllocConsole(); // For initial HOTKEY setup
        
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        

    // FILE *verify = fopen("config.txt", "rb");

    // int ch;
    // while ((ch = fgetc(verify)) != EOF)
    // {
    //     printf("%02X ", (unsigned char)ch);
    // }

    // printf("\n");
    // fclose(verify);

    //         printf("%u\n", screenMods);
    //         printf("%c\n", screenKey);
    //         printf("%u\n", windowMods);
    //         printf("%c\n", windowKey);
        RegisterHotkeyInteractive(
            HOTKEY_SCREEN_ID,
            "FULL SCREENSHOT HOTKEY",
            &screenMods, &screenKey);

        RegisterHotkeyInteractive(
            HOTKEY_WINDOW_ID,
            "CURRENT WINDOW SCREENSHOT HOTKEY",
            &windowMods, &windowKey);

        printf("BOTH KEYS REGISTERED SUCCESSFULLY.\nYOU CAN NOW CLOSE THIS CONSOLE WINDOW\n");
        saveConfig(screenMods, screenKey, windowMods, windowKey);
        FreeConsole();
    }


    // To prevent bloody scaling problems
    SetProcessDPIAware();

    // MOD_ALT      == Alt
    // MOD_CONTROL  == Ctrl
    // MOD_SHIFT    == Shift
    // MOD_WIN      == Windows key

    // MOD_CONTROL | MOD_ALT           == Ctrl + Alt
    // MOD_CONTROL | MOD_SHIFT         == Ctrl + Shift
    // MOD_ALT | MOD_SHIFT             == Alt + Shift
    // MOD_CONTROL | MOD_ALT | MOD_SHIFT

    CreateDirectory("screenshot_vault", NULL);

    MSG msg = {0};
    // Like pygame's input loo[]
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_HOTKEY)
        {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "screenshot_vault\\ss_%ld.bmp", (long)time(NULL));

            if (msg.wParam == HOTKEY_SCREEN_ID)
            {
                int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
                int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                CaptureRect(path, x, y, w, h);
                RunPythonPipeline(path);
            }
            else if (msg.wParam == HOTKEY_WINDOW_ID)
            {
                HWND hwnd = GetForegroundWindow(); // Simple AF
                if (hwnd)
                {
                    // COMPLETELY LIKE PYGAME LOL
                    RECT rect;
                    GetWindowRect(hwnd, &rect);

                    int x = rect.left;
                    int y = rect.top;
                    int w = rect.right - rect.left;
                    int h = rect.bottom - rect.top;

                    if (w > 0 && h > 0)
                    {
                        CaptureRect(path, x, y, w, h);
                        RunPythonPipeline(path);
                    }
                }
            }
        }
    }
    return 0;
}