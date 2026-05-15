#include "TrayIcon.h"
#include <sstream>
#include <string>

    static TrayIcon* g_trayInstance = nullptr;

    TrayIcon::TrayIcon(HINSTANCE hInstance, const std::wstring& tooltip)
        : m_hInstance(hInstance)
        , m_hwnd(nullptr)
        , m_parentHwnd(nullptr)
        , m_tooltip(tooltip)
        , m_created(false)
        , m_blinkIcon1(nullptr)
        , m_blinkIcon2(nullptr)
        , m_blinkState(false)
        , m_blinking(false)
    {
        g_trayInstance = this;
        ZeroMemory(&m_nid, sizeof(m_nid));
    }

    TrayIcon::~TrayIcon() {
        StopBlink();
        Remove();
        if (m_hwnd) DestroyWindow(m_hwnd);
    }

    bool TrayIcon::Create(HWND parentHwnd) {
        m_parentHwnd = parentHwnd;
        // 注册隐藏窗口类（用于接收托盘消息）
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = L"MiniCAD_TrayClass";
        RegisterClassEx(&wc);  // 已注册则忽略返回值

        m_hwnd = CreateWindowEx(0, L"MiniCAD_TrayClass", L"",
            0, 0, 0, 0, 0,
            HWND_MESSAGE,   // 消息专用窗口，不显示
            nullptr, m_hInstance, nullptr);
        if (!m_hwnd) return false;

        // 填写 NOTIFYICONDATA
        m_nid.cbSize = sizeof(m_nid);
        m_nid.hWnd = m_hwnd;
        m_nid.uID = ID_TRAY_ICON;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        // 使用系统默认应用图标，替换成你的图标资源
        m_nid.hIcon = (HICON)LoadImage(
            nullptr,              // hInstance 填 nullptr 表示从文件加载
            L"icons/logo.ico",    // 相对于 exe 所在目录的路径
            IMAGE_ICON,
            16, 16,               // 托盘图标尺寸，16x16 最合适
            LR_LOADFROMFILE       // ✅ 关键标志：从文件而不是资源加载
        );
        if (!m_nid.hIcon)
            m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

        wcsncpy_s(m_nid.szTip, m_tooltip.c_str(), _TRUNCATE);

        m_created = Shell_NotifyIcon(NIM_ADD, &m_nid);

        // 设置版本（支持气泡通知）
        m_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIcon(NIM_SETVERSION, &m_nid);

        return m_created;
    }

    void TrayIcon::Remove() {
        if (m_created) {
            Shell_NotifyIcon(NIM_DELETE, &m_nid);
            m_created = false;
        }
    }

    void TrayIcon::ShowBalloon(const std::wstring& title,
        const std::wstring& message,
        DWORD timeoutMs,
        DWORD infoFlags)
    {
        m_nid.uFlags |= NIF_INFO;
        m_nid.uTimeout = timeoutMs;
        m_nid.dwInfoFlags = infoFlags;
        wcsncpy_s(m_nid.szInfoTitle, title.c_str(), _TRUNCATE);
        wcsncpy_s(m_nid.szInfo, message.c_str(), _TRUNCATE);
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
        // 清除 NIF_INFO 防止每次修改都弹出
        m_nid.uFlags &= ~NIF_INFO;
    }

    void TrayIcon::SetIcon(HICON hIcon) {
        m_nid.hIcon = hIcon;
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    }

    void TrayIcon::SetTooltip(const std::wstring& tooltip) {
        wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    }

    // ── 闪烁（模拟QQ未读消息） ──────────────────────────
    void TrayIcon::StartBlink(HICON icon1, HICON icon2, int intervalMs) {
        m_blinkIcon1 = icon1;
        m_blinkIcon2 = icon2;
        m_blinking = true;
        m_blinkState = false;
        SetTimer(m_hwnd, 42, intervalMs, BlinkTimerProc);
    }

    void TrayIcon::StopBlink() {
        if (m_blinking) {
            KillTimer(m_hwnd, 42);
            m_blinking = false;
            // 恢复原图标
            if (m_blinkIcon1) SetIcon(m_blinkIcon1);
        }
    }

    void CALLBACK TrayIcon::BlinkTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD) {
        if (!g_trayInstance || !g_trayInstance->m_blinking) return;
        g_trayInstance->m_blinkState = !g_trayInstance->m_blinkState;
        g_trayInstance->SetIcon(g_trayInstance->m_blinkState
            ? g_trayInstance->m_blinkIcon2
            : g_trayInstance->m_blinkIcon1);
    }

    // ── 右键菜单 ────────────────────────────────────────
    void TrayIcon::ShowContextMenu() {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"显示 MiniCAD");
        AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CHECK, L"检查更新");
        AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出");

        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(m_hwnd);   // 必须，否则菜单不会自动关闭
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
            pt.x, pt.y, 0, m_hwnd, nullptr);
        DestroyMenu(hMenu);
    }

    // ── 消息处理 ────────────────────────────────────────
    void TrayIcon::OnTrayMessage(WPARAM wParam, LPARAM lParam) {
        UINT msg = LOWORD(lParam);
        switch (msg) {
        case WM_LBUTTONDBLCLK:   // 双击 → 显示主窗口
            if (m_parentHwnd)
            {
                ::ShowWindow(m_parentHwnd, SW_RESTORE);
                ::SetForegroundWindow(m_parentHwnd);
            }
            break;
        case WM_RBUTTONUP:        // 右键 → 弹出菜单
            ShowContextMenu();
            break;
        case NIN_BALLOONUSERCLICK: // 点击气泡通知
            if (m_parentHwnd)
            {
                ::ShowWindow(m_parentHwnd, SW_RESTORE);
                ::SetForegroundWindow(m_parentHwnd);
            }
            break;
        }
    }

    LRESULT CALLBACK TrayIcon::WndProc(HWND hwnd, UINT msg,
        WPARAM wParam, LPARAM lParam)
    {
        if (!g_trayInstance) return DefWindowProc(hwnd, msg, wParam, lParam);

        switch (msg) {
        case WM_TRAYICON:
            g_trayInstance->OnTrayMessage(wParam, lParam);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case ID_TRAY_EXIT:
                PostQuitMessage(0);
                break;
            case ID_TRAY_SHOW:
                // TODO: 显示你的主窗口
                ShowWindow(g_trayInstance->m_parentHwnd, SW_SHOW);
                break;
            case ID_TRAY_CHECK:
                g_trayInstance->CheckUpdate();
                break;
            }
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    std::wstring TrayIcon::HttpGet(const std::wstring& url) {
        std::string result;

        HINTERNET hInternet = InternetOpen(
            L"MiniCAD-Updater",
            INTERNET_OPEN_TYPE_PRECONFIG,
            nullptr, nullptr, 0);
        if (!hInternet) return L"";

        HINTERNET hConnect = InternetOpenUrl(
            hInternet,
            url.c_str(),
            nullptr, 0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE,  // HTTPS
            0);

        if (hConnect) {
            char buf[4096];
            DWORD bytesRead = 0;
            while (InternetReadFile(hConnect, buf, sizeof(buf) - 1, &bytesRead)
                && bytesRead > 0) {
                buf[bytesRead] = '\0';
                result += buf;
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);

        // 转换为 wstring
        return std::wstring(result.begin(), result.end());
    }

    // ── 从 GitHub API JSON 里提取 tag_name ───────────────────
    // 返回示例："1.2.0"（去掉前缀 'v'）
    std::wstring TrayIcon::ParseLatestVersion(const std::string& json) {
        // GitHub返回: {..., "tag_name": "v1.2.0", ...}
        const std::string key = "\"tag_name\":";
        size_t pos = json.find(key);
        if (pos == std::string::npos) return L"";

        pos = json.find('"', pos + key.size());   // 找到值的开头引号
        if (pos == std::string::npos) return L"";
        pos++;  // 跳过引号

        // 跳过可能的 'v' 前缀
        if (json[pos] == 'v') pos++;

        size_t end = json.find('"', pos);
        if (end == std::string::npos) return L"";

        std::string ver = json.substr(pos, end - pos);
        return std::wstring(ver.begin(), ver.end());
    }

    // ── 主逻辑：检查更新 ──────────────────────────────────────
    void TrayIcon::CheckUpdate() {
        // ✅ 替换成你自己的 GitHub 用户名/仓库名
        const std::wstring apiUrl =
            L"https://api.github.com/repos/qizhiwoniu/MiniCAD/releases/latest";

        // 先弹提示"正在检查"
        ShowBalloon(L"检查更新", L"正在连接 GitHub...", 2000, NIIF_INFO);

        std::wstring response = HttpGet(apiUrl);
        if (response.empty()) {
            ShowBalloon(L"检查更新失败", L"无法连接到 GitHub，请检查网络。",
                3000, NIIF_ERROR);
            return;
        }

        // 转回 string 用于解析
        std::string json(response.begin(), response.end());
        std::wstring latestVer = ParseLatestVersion(json);

        if (latestVer.empty()) {
            ShowBalloon(L"检查更新失败", L"无法解析版本信息。",
                3000, NIIF_ERROR);
            return;
        }

        if (latestVer == APP_VERSION) {
            // 已是最新
            ShowBalloon(L"已是最新版本",
                L"当前版本 v" APP_VERSION L" 已是最新。",
                3000, NIIF_INFO);
        }
        else {
            // 有新版本，询问是否打开下载页
            std::wstring msg = L"发现新版本 v" + latestVer +
                L"\n当前版本 v" APP_VERSION
                L"\n\n是否前往 GitHub 下载？";

            int ret = MessageBox(m_parentHwnd, msg.c_str(),
                L"MiniCAD 有新版本", MB_YESNO | MB_ICONINFORMATION);
            if (ret == IDYES) {
                // 打开浏览器跳转到 releases 页
                std::wstring releaseUrl =
                    L"https://github.com/qizhiwoniu/MiniCAD/releases/latest";
                ShellExecute(nullptr, L"open", releaseUrl.c_str(),
                    nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
    }