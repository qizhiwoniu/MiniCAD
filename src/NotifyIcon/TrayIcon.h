#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Shell32.lib")

#define APP_VERSION L"1.0.2"   // 你当前程序的版本号
#define WM_TRAYICON (WM_USER + 1)   // 自定义托盘消息
#define ID_TRAY_ICON 1001
#define ID_TRAY_EXIT 2001
#define ID_TRAY_SHOW 2002
#define ID_TRAY_CHECK 2003

class TrayIcon {
public:
    TrayIcon(HINSTANCE hInstance, const std::wstring& tooltip);
    ~TrayIcon();

    bool Create(HWND parentHwnd = nullptr);
    void Remove();

    // 显示气泡通知（类似QQ消息提示）
    void ShowBalloon(const std::wstring& title,
        const std::wstring& message,
        DWORD timeoutMs = 3000,
        DWORD infoFlags = NIIF_INFO);

    // 更新图标（比如未读消息时换红点图标）
    void SetIcon(HICON hIcon);
    void SetTooltip(const std::wstring& tooltip);

    // 闪烁效果（模拟QQ消息闪烁）
    void StartBlink(HICON icon1, HICON icon2, int intervalMs = 500);
    void StopBlink();

    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
        WPARAM wParam, LPARAM lParam);
    void OnTrayMessage(WPARAM wParam, LPARAM lParam);
    void ShowContextMenu();
    static void CALLBACK BlinkTimerProc(HWND hwnd, UINT msg,
        UINT_PTR id, DWORD time);
    void CheckUpdate();                          // 检查更新
    std::wstring HttpGet(const std::wstring& url); // 请求GitHub API
    std::wstring ParseLatestVersion(const std::string& json); // 解析版本号
    HINSTANCE       m_hInstance;
    HWND            m_hwnd;
    HWND            m_parentHwnd;
    NOTIFYICONDATA  m_nid;
    std::wstring    m_tooltip;
    bool            m_created;

    // 闪烁相关
    HICON   m_blinkIcon1;
    HICON   m_blinkIcon2;
    bool    m_blinkState;
    bool    m_blinking;
};