

#include <iostream>
#include <future>
#include <array>

#include <windows.h>


extern "C"  typedef int (*SetDCHU_DataPtr)(int command, byte* buffer, int length);

SetDCHU_DataPtr SetDCHU_Data;


void setBrightness(byte v) {
    std::array<byte, 4> buf{ v, 0, 0, 244 };
    SetDCHU_Data(103, buf.data(), 4);
}

HWND lastHwnd = NULL;

VOID CALLBACK WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {

    if (lastHwnd == hwnd || !hwnd)
        return;

    if (dwEvent == EVENT_SYSTEM_FOREGROUND || dwEvent == EVENT_SYSTEM_MINIMIZEEND || dwEvent == EVENT_SYSTEM_MINIMIZESTART) {

        if (!IsWindowVisible(hwnd))
            return;

        HICON hIcon = 0;
        if (!hIcon) SendMessageTimeoutA(hwnd, WM_GETICON, ICON_SMALL, NULL, SMTO_ABORTIFHUNG | SMTO_BLOCK, 30, (PDWORD_PTR)&hIcon);
        if (!hIcon) SendMessageTimeoutA(hwnd, WM_GETICON, ICON_BIG, NULL, SMTO_ABORTIFHUNG | SMTO_BLOCK, 30, (PDWORD_PTR)&hIcon);
        if (!hIcon) hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
        if (!hIcon) hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
        if (!hIcon) {
#ifndef NDEBUG
            std::cout << "Could not find icon" << std::endl;
#endif
            return;
        }

        ICONINFO iconinfo;
        if (!GetIconInfo(hIcon, &iconinfo)) {
#ifndef NDEBUG
            std::cout << "Could not get icon info" << std::endl;
#endif
            return;
        }
        HBITMAP& hbm = iconinfo.hbmColor;
        if (!hbm) {
#ifndef NDEBUG
            std::cout << "Could not get icon hbmColor" << std::endl;
#endif
            return;
        }

        BITMAP bm;
        if (!GetObject(hbm, sizeof(BITMAP), &bm)) {
#ifndef NDEBUG
            std::cout << "Could not get bitmap" << std::endl;
#endif
            return;
        }

        LONG pixelCount = bm.bmWidth * bm.bmHeight;

        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
        bmi.bmiHeader.biWidth = bm.bmWidth;
        bmi.bmiHeader.biHeight = -bm.bmHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = pixelCount * 4;

        auto lpPixels = std::vector<uint32_t>(pixelCount);
        if (!GetDIBits(GetDC(hwnd), hbm, 0, bm.bmHeight, lpPixels.data(), &bmi, DIB_RGB_COLORS)) {
#ifndef NDEBUG
            std::cout << "Could not copy icon image" << std::endl;
#endif
            return;
        }
        
        constexpr float GAMMA = 2;
        uint64_t rSum = 0, gSum = 0, bSum = 0;
        for (const auto& p : lpPixels)
            if (p >= 0xff000000) {
                uint64_t r = (p & 0xff0000) >> 16, g = (p & 0xff00) >> 8, b = p & 0xff;
                uint64_t min = (std::min)({ r, g, b });
                uint64_t max = (std::max)({ r, g, b });
                uint64_t weight = max - min;
                rSum += r * r * weight;
                gSum += g * g * weight;
                bSum += b * b * weight;
            }

        uint64_t max = (std::max)({ rSum, gSum, bSum, 255ULL });
        rSum = rSum * 255 / max;
        gSum = gSum * 255 / max;
        bSum = bSum * 255 / max;

        lastHwnd = hwnd;

        std::array<byte, 4> buffer = { gSum, rSum, bSum, 240 };
        SetDCHU_Data(103, buffer.data(), 4);

#ifndef NDEBUG
        std::array<char, 256> title{ 0 };
        GetWindowTextA(hwnd, title.data(), 256);

        printf("%02x%02x%02x %s\n", rSum, gSum, bSum, title.data());
#endif
    }
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    LPCWSTR mutexName = L"L0laapk3_ClevoKeyboardColor";
    HANDLE hHandle = CreateMutex(NULL, TRUE, mutexName);
    if (ERROR_ALREADY_EXISTS == GetLastError())
        return -1;

    auto hdl = LoadLibraryA("InsydeDCHU.dll");
    if (!hdl)
        return 1;

    SetDCHU_Data = reinterpret_cast<SetDCHU_DataPtr>(GetProcAddress(hdl, "SetDCHU_Data"));
    if (!SetDCHU_Data)
        return 2;


    setBrightness(255);

    SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_MINIMIZEEND, NULL, WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT);

    WinEventProcCallback(NULL, EVENT_SYSTEM_FOREGROUND, GetForegroundWindow(), 0, 0, NULL, NULL);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        WinEventProcCallback(NULL, EVENT_SYSTEM_FOREGROUND, GetForegroundWindow(), 0, 0, NULL, NULL);
    }

    FreeLibrary(hdl);

}
