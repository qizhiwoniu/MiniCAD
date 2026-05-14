#include "pch.h"
#include "MainWindow.h"

using namespace MiniCAD;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, PSTR, int)
{
    MainWindow mainWindow;
    mainWindow.Initialize(L"MiniCAD", 600, 400);
    mainWindow.Run();
    return 0;
}
 
//int main(int argc, char* argv[]) // 小写
//{
//    MainWindow mainWindow;
//    mainWindow.Initialize(L"MiniCAD", 600, 400);
//    mainWindow.Run();
//    return 0;
//}
