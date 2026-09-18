#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <string>
#include <vector>
#include <cstdint>

enum class VisualizerMode {
    EQUALIZER_BARS = 0,
    WAVEFORM_CURVE = 1,
    MIRRORED_WAVE = 2
};

struct ColorStop {
    float position; // 0.0f to 1.0f
    D2D1_COLOR_F color;
    std::string hex;
};

struct AppConfig {
    // Window settings
    std::string position_mode = "bottom_screen";
    float height_cm = 3.0f; // Range: 1.5f - 6.0f (sleek bottom overlay)
    int max_height_px = 190;
    bool dock_above_taskbar = true;
    int taskbar_margin_px = 2; // Clean margin above taskbar
    int target_monitor_index = 0;
    int fps_cap = 120;
    bool click_through = true;

    // Audio settings
    int sample_rate = 48000;
    int fft_size = 1024;
    int num_bands = 64;
    float min_freq_hz = 20.0f;
    float max_freq_hz = 20000.0f;
    float attack_time_ms = 12.0f;
    float decay_time_ms = 140.0f;
    float sensitivity = 1.25f;

    // Visualizer settings
    VisualizerMode mode = VisualizerMode::EQUALIZER_BARS;
    float bar_width_ratio = 0.75f;
    float bar_spacing_px = 3.0f;
    float corner_radius_px = 2.5f;
    bool peak_cap_enabled = true;
    float peak_hold_ms = 220.0f;
    float peak_gravity = 980.0f;

    // Waveform settings
    float wave_thickness_px = 2.5f;
    bool fill_under_wave = true;
    float fill_opacity = 0.30f;

    // Color mixer settings
    std::string blend_mode = "frequency_split";
    std::vector<ColorStop> palette;
    float glow_boost = 0.45f;
    float base_alpha = 0.95f;
};
