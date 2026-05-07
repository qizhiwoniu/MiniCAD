#include "MainWindow.h"

// 1. wWinMain ==  win32 入口 add_executable(YGsoftware WIN32 ${SOURCES}) 没有控制台
// 2. main = 控制台入口 add_executable(YGsoftware ${SOURCES})
// 使用 wWinMain 就没有控制台了 
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{ 
    YGsoftware::MainWindow mainWindow; // YGsoftware:: 是命名空间
    mainWindow.Initialize(L"YG software", 720, 480);
    mainWindow.Run(); 
	return 0;
}