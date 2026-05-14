#include "pch.h"
#include "MainWindow.h"
#include "NotifyIcon/TrayIcon.h"

using namespace MiniCAD;

//int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, PSTR, int)
//{
//    //1. 先创建托盘图标
//    TrayIcon tray(hInst, L"MiniCAD - 就绪");
//    tray.Create();
//    //2. 用系统内置图标（暂时，之后换成你自己的.ico）
//    HICON iconNormal = LoadIcon(nullptr, IDI_APPLICATION);  // 普通状态
//    HICON iconAlert = LoadIcon(nullptr, IDI_INFORMATION);  // 有消息时
//    //3. 初始化主窗口
//    MainWindow mainWindow;
//    mainWindow.Initialize(L"MiniCAD", 600, 400);
//    //4. 示例：窗口启动后弹出气泡通知
//    tray.ShowBalloon(L"MiniCAD 已启动", L"程序正在运行中...");
//
//    mainWindow.Run();
//    return 0;
//
//
//    //tray.StartBlink(iconNormal, iconAlert, 500);  // 500ms 闪一次
//
//    // 消息已读，停止闪烁：
//    //tray.StopBlink();
//}
 
int main(int argc, char* argv[]) 
{
    //1. 先创建托盘图标
    TrayIcon tray(hInst, L"MiniCAD - 就绪");
    tray.Create();
    //2. 用系统内置图标（暂时，之后换成你自己的.ico）
    HICON iconNormal = LoadIcon(nullptr, IDI_APPLICATION);  // 普通状态
    HICON iconAlert = LoadIcon(nullptr, IDI_INFORMATION);  // 有消息时
    //3. 初始化主窗口
    MainWindow mainWindow;
    mainWindow.Initialize(L"MiniCAD", 600, 400);
    //4. 示例：窗口启动后弹出气泡通知
    tray.ShowBalloon(L"MiniCAD 已启动", L"程序正在运行中...");

    mainWindow.Run();
    return 0;


    //tray.StartBlink(iconNormal, iconAlert, 500);  // 500ms 闪一次

    // 消息已读，停止闪烁：
    //tray.StopBlink();
}
