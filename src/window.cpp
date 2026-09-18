#include "window.hpp"
#include <cmath>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

static WindowManager* g_window_manager = nullptr;

WindowManager::WindowManager()
    : m_hwnd(NULL), m_hinstance(NULL), m_width(1920), m_height(190),
      m_visible(true) {
    g_window_manager = this;
    ZeroMemory(&m_nid, sizeof(m_nid));
}

WindowManager::~WindowManager() {
    Destroy();
}

void WindowManager::SetCallbacks(ModeChangeCallback mode_cb, ExitCallback exit_cb) {
    m_mode_cb = mode_cb;
    m_exit_cb = exit_cb;
}

bool WindowManager::Create(HINSTANCE hInstance, const AppConfig& config) {
    m_hinstance = hInstance;

    const wchar_t CLASS_NAME[] = L"AetherWaveVisualizerClass";

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowManager::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassExW(&wc);

    DWORD ex_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    if (config.click_through) {
        ex_style |= WS_EX_TRANSPARENT;
    }

    // Temporary placeholder size, will be immediately adjusted in UpdatePosition
    m_hwnd = CreateWindowExW(
        ex_style,
        CLASS_NAME,
        L"AetherWave Audio Visualizer",
        WS_POPUP,
        0, 0, m_width, m_height,
        NULL, NULL, hInstance, NULL
    );

    if (!m_hwnd) return false;

    UpdatePosition(config);

    // Register hotkey Ctrl + Shift + V
    RegisterHotKey(m_hwnd, HOTKEY_TOGGLE_ID, MOD_CONTROL | MOD_SHIFT, 'V');

    SetupTrayIcon();

    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    return true;
}

void WindowManager::UpdatePosition(const AppConfig& config) {
    if (!m_hwnd) return;

    HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfoW(hMon, &mi);

    int screen_w = mi.rcMonitor.right - mi.rcMonitor.left;
    int screen_h = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // Get DPI for accurate 4-6cm conversion
    UINT dpi = 96;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef UINT(WINAPI* GetDpiForWindowFunc)(HWND);
        GetDpiForWindowFunc pGetDpi = (GetDpiForWindowFunc)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpi) {
            dpi = pGetDpi(m_hwnd);
            if (dpi == 0) dpi = 96;
        }
    }

    // Physical CM to Pixels calculation: (cm / 2.54) * DPI
    float clamped_cm = std::clamp(config.height_cm, 1.5f, 6.0f);
    m_height = static_cast<int>(std::round((clamped_cm / 2.54f) * dpi));
    m_width = screen_w;

    int pos_x = mi.rcMonitor.left;
    
    // Start with work area bottom (which automatically excludes standard taskbar)
    int target_bottom = mi.rcWork.bottom;

    // Detect live Taskbar position to ensure zero overlap even with auto-hide taskbar
    if (config.dock_above_taskbar) {
        HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
        if (hTaskbar) {
            RECT rcTaskbar;
            if (GetWindowRect(hTaskbar, &rcTaskbar)) {
                // If taskbar is on the screen and active at bottom
                if (rcTaskbar.top > mi.rcMonitor.top && rcTaskbar.top < mi.rcMonitor.bottom) {
                    target_bottom = std::min(target_bottom, static_cast<int>(rcTaskbar.top));
                }
            }
        }
    }

    // Apply safety margin (2px) so visualizer never touches the taskbar edge
    int margin = config.taskbar_margin_px;
    int pos_y = target_bottom - m_height - margin;

    SetWindowPos(m_hwnd, HWND_TOPMOST, pos_x, pos_y, m_width, m_height,
                 SWP_NOACTIVATE | (m_visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void WindowManager::CheckAndRepositionIfNeeded(const AppConfig& config) {
    if (!m_hwnd) return;

    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hTaskbar) return;

    RECT rcTaskbar;
    GetWindowRect(hTaskbar, &rcTaskbar);

    RECT rcWnd;
    GetWindowRect(m_hwnd, &rcWnd);

    // If taskbar moved or window bottom overlaps taskbar, reposition!
    int expected_bottom = rcTaskbar.top - config.taskbar_margin_px;
    if (std::abs(rcWnd.bottom - expected_bottom) > 4) {
        UpdatePosition(config);
    }
}

void WindowManager::ToggleVisibility() {
    m_visible = !m_visible;
    if (m_hwnd) {
        ShowWindow(m_hwnd, m_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
}

void WindowManager::SetupTrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"AetherWave Audio Visualizer (Ctrl+Shift+V)");

    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void WindowManager::RemoveTrayIcon() {
    if (m_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_nid.hWnd = NULL;
    }
}

void WindowManager::ShowTrayMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 101, L"Mode: Equalizer Bars");
    AppendMenuW(hMenu, MF_STRING, 102, L"Mode: Waveform Curve");
    AppendMenuW(hMenu, MF_STRING, 103, L"Mode: Mirrored Wave");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 104, m_visible ? L"Hide Visualizer (Ctrl+Shift+V)" : L"Show Visualizer (Ctrl+Shift+V)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 105, L"Exit AetherWave");

    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                             pt.x, pt.y, 0, m_hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == 101 && m_mode_cb) {
        m_mode_cb(VisualizerMode::EQUALIZER_BARS);
    } else if (cmd == 102 && m_mode_cb) {
        m_mode_cb(VisualizerMode::WAVEFORM_CURVE);
    } else if (cmd == 103 && m_mode_cb) {
        m_mode_cb(VisualizerMode::MIRRORED_WAVE);
    } else if (cmd == 104) {
        ToggleVisibility();
    } else if (cmd == 105 && m_exit_cb) {
        m_exit_cb();
    }
}

void WindowManager::Destroy() {
    RemoveTrayIcon();
    if (m_hwnd) {
        UnregisterHotKey(m_hwnd, HOTKEY_TOGGLE_ID);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!g_window_manager) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_NCHITTEST:
            return HTTRANSPARENT; // Click-through 100%

        case WM_ERASEBKGND:
            return 1;

        case WM_HOTKEY:
            if (wParam == HOTKEY_TOGGLE_ID) {
                g_window_manager->ToggleVisibility();
            }
            return 0;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                g_window_manager->ShowTrayMenu();
            } else if (lParam == WM_LBUTTONDBLCLK) {
                g_window_manager->ToggleVisibility();
            }
            return 0;

        case WM_DISPLAYCHANGE: {
            AppConfig dummy;
            g_window_manager->UpdatePosition(dummy);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
