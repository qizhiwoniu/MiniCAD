#include "pch.h"
#include "MainWindow.h"
#include "NotifyIcon/TrayIcon.h"

using namespace MiniCAD;

static int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, PSTR, int)
{
    
    //3. 初始化主窗口
    MainWindow mainWindow;
    mainWindow.Initialize(L"MiniCAD", 600, 400);
    //4. 示例：窗口启动后弹出气泡通知
    //tray.ShowBalloon(L"MiniCAD 已启动", L"程序正在运行中...");

    mainWindow.Run();
    return 0;
}

//int main(int argc, char* argv[])
//{
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//
//    MainWindow mainWindow;
//    mainWindow.Initialize(L"MiniCAD", 600, 400);
//
//    mainWindow.Run();
//    return 0;
//}
