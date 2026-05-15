#include "pch.h"
#include "MainWindow.h"
#include "NotifyIcon/TrayIcon.h"

using namespace MiniCAD;

#ifdef USE_WIN32 

static int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, PSTR, int)
{
    MainWindow mainWindow;
    mainWindow.Initialize(L"MiniCAD", 600, 400);

    mainWindow.Run();
    return 0;

}

#else

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);


    MainWindow mainWindow;
    mainWindow.Initialize(L"MiniCAD", 600, 400);

    mainWindow.Run();
    return 0;
}

#endif