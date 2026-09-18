#pragma once
#include "types.hpp"
#include <windows.h>
#include <shellapi.h>
#include <functional>

#define WM_TRAYICON (WM_USER + 1)
#define HOTKEY_TOGGLE_ID 1001

class WindowManager {
public:
    using ModeChangeCallback = std::function<void(VisualizerMode mode)>;
    using ExitCallback = std::function<void()>;

    WindowManager();
    ~WindowManager();

    bool Create(HINSTANCE hInstance, const AppConfig& config);
    void Destroy();

    HWND GetHWND() const { return m_hwnd; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsVisible() const { return m_visible; }

    void ToggleVisibility();
    void SetCallbacks(ModeChangeCallback mode_cb, ExitCallback exit_cb);

    void UpdatePosition(const AppConfig& config);
    void CheckAndRepositionIfNeeded(const AppConfig& config);

    // Static window procedure
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void SetupTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();

    HWND m_hwnd;
    HINSTANCE m_hinstance;
    int m_width;
    int m_height;
    bool m_visible;

    NOTIFYICONDATAW m_nid;
    ModeChangeCallback m_mode_cb;
    ExitCallback m_exit_cb;
};
