#include "config_manager.hpp"
#include "color_mixer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ConfigManager::ConfigManager() : m_watching(false) {}

ConfigManager::~ConfigManager() {
    StopWatcher();
}

static std::string ExtractStringVal(const std::string& content, const std::string& key, const std::string& default_val) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return default_val;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return default_val;

    size_t quote1 = content.find('\"', colon);
    if (quote1 == std::string::npos) return default_val;

    size_t quote2 = content.find('\"', quote1 + 1);
    if (quote2 == std::string::npos) return default_val;

    return content.substr(quote1 + 1, quote2 - quote1 - 1);
}

static float ExtractFloatVal(const std::string& content, const std::string& key, float default_val) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return default_val;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return default_val;

    size_t start = content.find_first_of("-0123456789.", colon);
    if (start == std::string::npos) return default_val;

    size_t end = content.find_first_not_of("0123456789.eE-+", start);
    std::string num_str = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);

    try {
        return std::stof(num_str);
    } catch (...) {
        return default_val;
    }
}

static int ExtractIntVal(const std::string& content, const std::string& key, int default_val) {
    return static_cast<int>(ExtractFloatVal(content, key, static_cast<float>(default_val)));
}

static bool ExtractBoolVal(const std::string& content, const std::string& key, bool default_val) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return default_val;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return default_val;

    size_t next_comma = content.find(',', colon);
    size_t next_brace = content.find('}', colon);
    size_t end = std::min(next_comma, next_brace);

    std::string val = content.substr(colon + 1, end - (colon + 1));
    if (val.find("true") != std::string::npos) return true;
    if (val.find("false") != std::string::npos) return false;
    return default_val;
}

bool ConfigManager::LoadOrCreate(const std::string& file_path, AppConfig& out_config) {
    if (!fs::exists(file_path)) {
        // Create default config
        Save(file_path, out_config);
        return true;
    }

    std::ifstream file(file_path);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Parse Window settings
    out_config.height_cm = std::clamp(ExtractFloatVal(content, "height_cm", 2.8f), 1.5f, 6.0f);
    out_config.dock_above_taskbar = ExtractBoolVal(content, "dock_above_taskbar", true);
    out_config.taskbar_margin_px = ExtractIntVal(content, "taskbar_margin_px", 3);
    out_config.fps_cap = ExtractIntVal(content, "fps_cap", 120);
    out_config.click_through = ExtractBoolVal(content, "click_through", true);

    // Parse Audio settings
    out_config.sample_rate = ExtractIntVal(content, "sample_rate", 48000);
    out_config.fft_size = ExtractIntVal(content, "fft_size", 1024);
    out_config.num_bands = std::clamp(ExtractIntVal(content, "num_bands", 64), 16, 128);
    out_config.attack_time_ms = ExtractFloatVal(content, "attack_time_ms", 12.0f);
    out_config.decay_time_ms = ExtractFloatVal(content, "decay_time_ms", 140.0f);
    out_config.sensitivity = ExtractFloatVal(content, "sensitivity_multiplier", 1.25f);

    // Parse Visualizer settings
    std::string mode_str = ExtractStringVal(content, "mode", "equalizer_bars");
    if (mode_str == "waveform_curve") {
        out_config.mode = VisualizerMode::WAVEFORM_CURVE;
    } else if (mode_str == "mirrored_wave") {
        out_config.mode = VisualizerMode::MIRRORED_WAVE;
    } else {
        out_config.mode = VisualizerMode::EQUALIZER_BARS;
    }

    out_config.bar_spacing_px = ExtractFloatVal(content, "bar_spacing_px", 3.0f);
    out_config.corner_radius_px = ExtractFloatVal(content, "corner_radius_px", 2.5f);
    out_config.peak_cap_enabled = ExtractBoolVal(content, "peak_cap_enabled", true);
    out_config.wave_thickness_px = ExtractFloatVal(content, "line_thickness_px", 2.5f);
    out_config.fill_under_wave = ExtractBoolVal(content, "fill_under_wave", true);
    out_config.fill_opacity = ExtractFloatVal(content, "fill_opacity", 0.30f);

    // Parse Color Mixer settings
    out_config.blend_mode = ExtractStringVal(content, "blend_mode", "frequency_split");
    out_config.glow_boost = ExtractFloatVal(content, "amplitude_glow_boost", 0.45f);
    out_config.base_alpha = ExtractFloatVal(content, "base_alpha", 0.95f);

    // Parse palette stops
    size_t palette_pos = content.find("\"color_palette\"");
    if (palette_pos != std::string::npos) {
        size_t arr_start = content.find('[', palette_pos);
        size_t arr_end = content.find(']', arr_start);
        if (arr_start != std::string::npos && arr_end != std::string::npos) {
            std::string pal_content = content.substr(arr_start, arr_end - arr_start + 1);
            std::vector<ColorStop> stops;

            size_t cur = 0;
            while ((cur = pal_content.find('{', cur)) != std::string::npos) {
                size_t obj_end = pal_content.find('}', cur);
                if (obj_end == std::string::npos) break;

                std::string obj_str = pal_content.substr(cur, obj_end - cur + 1);
                float stop_pos = ExtractFloatVal(obj_str, "stop", 0.0f);
                std::string hex = ExtractStringVal(obj_str, "hex", "#FFFFFF");

                stops.push_back({ stop_pos, ColorMixer::HexToColor(hex), hex });
                cur = obj_end + 1;
            }

            if (stops.size() >= 2) {
                out_config.palette = stops;
            }
        }
    }

    return true;
}

bool ConfigManager::Save(const std::string& file_path, const AppConfig& config) {
    fs::path p(file_path);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }

    std::ofstream out(file_path);
    if (!out.is_open()) return false;

    std::string mode_str = "equalizer_bars";
    if (config.mode == VisualizerMode::WAVEFORM_CURVE) mode_str = "waveform_curve";
    else if (config.mode == VisualizerMode::MIRRORED_WAVE) mode_str = "mirrored_wave";

    out << "{\n";
    out << "  \"window\": {\n";
    out << "    \"position\": \"bottom_screen\",\n";
    out << "    \"height_cm\": " << config.height_cm << ",\n";
    out << "    \"dock_above_taskbar\": " << (config.dock_above_taskbar ? "true" : "false") << ",\n";
    out << "    \"taskbar_margin_px\": " << config.taskbar_margin_px << ",\n";
    out << "    \"fps_cap\": " << config.fps_cap << ",\n";
    out << "    \"click_through\": " << (config.click_through ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"audio\": {\n";
    out << "    \"sample_rate\": " << config.sample_rate << ",\n";
    out << "    \"fft_size\": " << config.fft_size << ",\n";
    out << "    \"num_bands\": " << config.num_bands << ",\n";
    out << "    \"attack_time_ms\": " << config.attack_time_ms << ",\n";
    out << "    \"decay_time_ms\": " << config.decay_time_ms << ",\n";
    out << "    \"sensitivity_multiplier\": " << config.sensitivity << "\n";
    out << "  },\n";
    out << "  \"visualizer\": {\n";
    out << "    \"mode\": \"" << mode_str << "\",\n";
    out << "    \"bar_spacing_px\": " << config.bar_spacing_px << ",\n";
    out << "    \"corner_radius_px\": " << config.corner_radius_px << ",\n";
    out << "    \"peak_cap_enabled\": " << (config.peak_cap_enabled ? "true" : "false") << ",\n";
    out << "    \"line_thickness_px\": " << config.wave_thickness_px << ",\n";
    out << "    \"fill_under_wave\": " << (config.fill_under_wave ? "true" : "false") << ",\n";
    out << "    \"fill_opacity\": " << config.fill_opacity << "\n";
    out << "  },\n";
    out << "  \"color_mixer\": {\n";
    out << "    \"blend_mode\": \"" << config.blend_mode << "\",\n";
    out << "    \"color_palette\": [\n";
    out << "      { \"stop\": 0.00, \"hex\": \"#FF1A4B\", \"label\": \"Crimson Bass\" },\n";
    out << "      { \"stop\": 0.30, \"hex\": \"#FF5722\", \"label\": \"Orange Mid-Bass\" },\n";
    out << "      { \"stop\": 0.65, \"hex\": \"#8B5CF6\", \"label\": \"Electric Violet Mids\" },\n";
    out << "      { \"stop\": 1.00, \"hex\": \"#00A3FF\", \"label\": \"Azure Neon Treble\" }\n";
    out << "    ],\n";
    out << "    \"amplitude_glow_boost\": " << config.glow_boost << ",\n";
    out << "    \"base_alpha\": " << config.base_alpha << "\n";
    out << "  }\n";
    out << "}\n";

    return true;
}

void ConfigManager::StartWatcher(const std::string& file_path, ConfigChangedCallback callback) {
    if (m_watching) return;
    m_callback = callback;
    m_watching = true;

    fs::path p(file_path);
    std::string dir = p.has_parent_path() ? p.parent_path().string() : ".";
    std::string filename = p.filename().string();

    m_thread = std::thread(&ConfigManager::WatcherThread, this, dir, filename);
}

void ConfigManager::StopWatcher() {
    if (!m_watching) return;
    m_watching = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ConfigManager::WatcherThread(std::string directory, std::string filename) {
    std::wstring wdir(directory.begin(), directory.end());

    HANDLE hDir = CreateFileW(
        wdir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) return;

    BYTE buffer[1024];
    DWORD bytesReturned = 0;

    while (m_watching) {
        if (ReadDirectoryChangesW(
                hDir, buffer, sizeof(buffer), FALSE,
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned, NULL, NULL)) {
            
            // Debounce delay to let editor finish writing file
            Sleep(80);

            AppConfig updated_config;
            std::string full_path = directory + "/" + filename;
            if (LoadOrCreate(full_path, updated_config)) {
                if (m_callback) {
                    m_callback(updated_config);
                }
            }
        }
        Sleep(100);
    }

    CloseHandle(hDir);
}
