#include "pch.h"
#include "MainWindow.h"
using namespace YGsoftware;
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{

	YGsoftware::MainWindow mainWindow;

	mainWindow.Initialize(L"YGsoftware",600,400);
	mainWindow.Run();
	return 0;
}
