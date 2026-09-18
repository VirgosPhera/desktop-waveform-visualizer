#include "types.hpp"
#include "config_manager.hpp"
#include "color_mixer.hpp"
#include "dsp.hpp"
#include "audio_capture.hpp"
#include "renderer.hpp"
#include "window.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <iostream>
#include <mutex>
#include <filesystem>

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 0. Enforce Single Instance on Windows: Named Mutex
    HANDLE hSingleInstanceMutex = CreateMutexW(NULL, TRUE, L"Local\\AetherWave_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is already active: unhide existing window and exit immediately
        HWND hExisting = FindWindowW(L"AetherWaveVisualizerClass", L"AetherWave Audio Visualizer");
        if (hExisting) {
            ShowWindow(hExisting, SW_SHOWNOACTIVATE);
        }
        if (hSingleInstanceMutex) {
            CloseHandle(hSingleInstanceMutex);
        }
        return 0;
    }
#else
int main(int argc, char* argv[]) {
    // 0. Enforce Single Instance on Linux / macOS: flock on runtime lockfile
    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    std::string lock_path = (runtime_dir && strlen(runtime_dir) > 0)
        ? std::string(runtime_dir) + "/aetherwave.lock"
        : "/tmp/aetherwave.lock";

    int lock_fd = open(lock_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (lock_fd >= 0) {
        if (flock(lock_fd, LOCK_EX | LOCK_NB) != 0) {
            std::cerr << "[AetherWave] Another instance is already running. Exiting.\n";
            close(lock_fd);
            return 0;
        }
    }
#endif

    // Enable Per-Monitor V2 DPI awareness
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
        SetProcessDpiAwarenessContextFunc pSetDpi = 
            (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi) {
            pSetDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // 1. Load Configuration (Resolve path relative to exe)
    wchar_t exe_path_w[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path_w, MAX_PATH);
    std::filesystem::path exe_dir = std::filesystem::path(exe_path_w).parent_path();
    std::filesystem::path config_path_fs = exe_dir / "config" / "config.json";
    if (!std::filesystem::exists(config_path_fs) && exe_dir.filename() == "bin") {
        config_path_fs = exe_dir.parent_path() / "config" / "config.json";
    }
    std::string config_path = config_path_fs.string();

    AppConfig config;
    ConfigManager config_mgr;
    config_mgr.LoadOrCreate(config_path, config);

    // 2. Setup Color Mixer
    ColorMixer color_mixer;
    if (!config.palette.empty()) {
        color_mixer.SetPalette(config.palette);
    } else {
        color_mixer.SetDefaultPalette();
    }

    // 3. Setup Window Manager
    WindowManager window_mgr;
    if (!window_mgr.Create(hInstance, config)) {
        CoUninitialize();
        return 1;
    }

    // 4. Setup Direct2D Renderer
    VisualizerRenderer renderer;
    if (!renderer.Initialize(window_mgr.GetHWND(), window_mgr.GetWidth(), window_mgr.GetHeight())) {
        window_mgr.Destroy();
        CoUninitialize();
        return 1;
    }

    // 5. Setup DSP Engine
    DspEngine dsp(config.fft_size, config.sample_rate, config.num_bands);

    std::mutex audio_mutex;
    // 6. Setup and Start WASAPI Loopback Capture
    AudioCapture audio_capture;
    audio_capture.Start([&](const float* samples, size_t count) {
        std::lock_guard<std::mutex> lock(audio_mutex);
        dsp.ProcessSamples(samples, count);
    });

    bool running = true;

    // Window callbacks
    window_mgr.SetCallbacks(
        [&](VisualizerMode mode) {
            config.mode = mode;
        },
        [&]() {
            running = false;
        }
    );

    // Config Watcher callback
    config_mgr.StartWatcher(config_path, [&](const AppConfig& new_config) {
        config = new_config;
        if (!config.palette.empty()) {
            color_mixer.SetPalette(config.palette);
        }
        window_mgr.UpdatePosition(config);
        renderer.Resize(window_mgr.GetWidth(), window_mgr.GetHeight());
    });

    // High-Resolution Timer for smooth 60-144 FPS
    LARGE_INTEGER freq, prev_time, curr_time;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev_time);

    float target_frame_time = 1.0f / static_cast<float>(config.fps_cap);

    MSG msg = { 0 };
    while (running) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!running) break;

        QueryPerformanceCounter(&curr_time);
        float dt = static_cast<float>(curr_time.QuadPart - prev_time.QuadPart) / freq.QuadPart;
        prev_time = curr_time;

        if (dt > 0.1f) dt = 0.1f; // Clamp delta time spikes

        // Update DSP
        {
            std::lock_guard<std::mutex> lock(audio_mutex);
            dsp.Update(dt, config);
        }

        static int frame_counter = 0;
        if (++frame_counter % 30 == 0) {
            window_mgr.CheckAndRepositionIfNeeded(config);
        }

        // Render Frame
        if (window_mgr.IsVisible()) {
            renderer.Render(
                dsp.GetBands(),
                dsp.GetPeakCaps(),
                dsp.GetWaveformPoints(),
                dsp.GetRms(),
                config,
                color_mixer
            );
        }

        // Frame rate limiter
        LARGE_INTEGER frame_end;
        QueryPerformanceCounter(&frame_end);
        float frame_duration = static_cast<float>(frame_end.QuadPart - curr_time.QuadPart) / freq.QuadPart;
        if (frame_duration < target_frame_time) {
            DWORD sleep_ms = static_cast<DWORD>((target_frame_time - frame_duration) * 1000.0f);
            if (sleep_ms > 0) {
                Sleep(sleep_ms);
            }
        }
    }

    // Cleanup
    config_mgr.StopWatcher();
    audio_capture.Stop();
    renderer.Shutdown();
    window_mgr.Destroy();
    CoUninitialize();

    if (hSingleInstanceMutex) {
        ReleaseMutex(hSingleInstanceMutex);
        CloseHandle(hSingleInstanceMutex);
    }

    return 0;
}
