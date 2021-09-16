

#include <iostream>
#include <future>
#include <array>
#include <thread>
#include <chrono>

#include <windows.h>


extern "C" typedef int (*SetDCHU_DataPtr)(int command, byte* buffer, int length);

SetDCHU_DataPtr SetDCHU_Data;


struct Color {
    byte r, g, b;
};

void setBrightness(byte v) {
    std::array<byte, 4> buf{ v, 0, 0, 244 };
    SetDCHU_Data(103, buf.data(), 4);
}

void setColor(Color c) {
    std::array<byte, 4> buffer = { c.g, c.r, c.b, 240 };
    SetDCHU_Data(103, buffer.data(), 4);
}


int main() {
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

    const std::array<Color, 7> colorList{{
        { 255, 0, 0 },
        { 255, 127, 0 },
        { 255, 255, 0 },
        { 0, 255, 0 },
        { 0, 255, 255 },
        { 0, 0, 255 },
        { 255, 0, 255 },
    }};
    int colorIndex = 0;
    while (true) {
        setColor(colorList[colorIndex]);
        colorIndex = (colorIndex + 1) % colorList.size();
        for (int brightness = 0; brightness < 256; brightness += 8) {
            setBrightness(brightness);
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
        for (int brightness = 255; brightness >= 0; brightness -= 8) {
            setBrightness(brightness);
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main();
}
