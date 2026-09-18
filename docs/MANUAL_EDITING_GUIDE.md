# AetherWave — Manual Editing & Developer Customization Guide

> **Author & Creator:** Amir (Only-One-Kind)  
> **Project:** AetherWave Desktop Audio Visualizer  
> **Target Audience:** Developers, tinkers, modders, and audio enthusiasts wanting to customize, tweak, or extend AetherWave.

---

## 📑 Table of Contents
1. [Overview](#1-overview)
2. [No-Code Customization (Live JSON Config)](#2-no-code-customization-live-json-config)
   - [File Locations](#file-locations)
   - [Window & Positioning Parameters](#window--positioning-parameters)
   - [Audio Sensitivity & FFT Parameters](#audio-sensitivity--fft-parameters)
   - [Visualizer Modes & Physics Parameters](#visualizer-modes--physics-parameters)
   - [Color Mixer & Palette Presets](#color-mixer--palette-presets)
3. [Source Code Customization (C++20)](#3-source-code-customization-c20)
   - [Project Directory Structure](#project-directory-structure)
   - [How to Tweak Audio DSP & Vocal Curves (`src/dsp.cpp`)](#how-to-tweak-audio-dsp--vocal-curves-srcdspcpp)
   - [How to Customize Waveform Curve Smoothing (`src/renderer.cpp`)](#how-to-customize-waveform-curve-smoothing-srcrenderercpp)
   - [How to Customize Equalizer Bars & Peak Gravity (`src/renderer.cpp`)](#how-to-customize-equalizer-bars--peak-gravity-srcrenderercpp)
   - [How to Add a Brand New Visualizer Mode](#how-to-add-a-brand-new-visualizer-mode-step-by-step)
   - [How to Modify Window Click-Through & Taskbar Tracking (`src/window.cpp`)](#how-to-modify-window-click-through--taskbar-tracking-srcwindowcpp)
4. [Compiling & Rebuilding](#4-compiling--rebuilding)
   - [Windows (MSVC)](#windows-msvc)
   - [Linux (Ubuntu/Debian/Arch/Fedora)](#linux-ubuntudebianarchfedora)
   - [macOS (Apple Silicon & Intel)](#macos-apple-silicon--intel)
5. [Author & Credits](#5-author--credits)

---

## 1. Overview

AetherWave is engineered to be **100% hackable and modifiable**. Whether you just want to tweak colors, increase bar heights, and adjust vocal responsiveness without touching a compiler, or you want to write your own custom Direct2D / Cairo rendering algorithms, this guide covers everything you need.

---

## 2. No-Code Customization (Live JSON Config)

AetherWave includes a built-in filesystem watcher powered by `ReadDirectoryChangesW` (Windows) and runtime config parsers. When you edit and save the configuration file, **changes are hot-reloaded within 50 milliseconds without restarting the application!**

### File Locations
- **Development config:** `config/config.json`
- **Runtime binary config:** `bin/config/config.json`

*(Tip: If running from `bin/AetherWave.exe`, edit `bin/config/config.json`. If building from source, edit `config/config.json`.)*

### Window & Positioning Parameters

```json
"window": {
  "position": "bottom_screen",
  "height_cm": 2.8,
  "dock_above_taskbar": true,
  "taskbar_margin_px": 3,
  "fps_cap": 120,
  "click_through": true
}
```

- `height_cm` *(float, default: `2.8`)*: Physical height of the visualizer on your monitor in centimeters (range: `1.5` to `6.0`). Automatically converted to real pixels based on your display's live DPI.
- `dock_above_taskbar` *(bool, default: `true`)*:
  - `true`: Automatically finds the Windows Taskbar (`Shell_TrayWnd`), panel (Linux), or Dock (macOS) and floats strictly above it.
  - `false`: Glues the visualizer to the absolute bottom edge of the monitor (`Y = ScreenHeight`).
- `taskbar_margin_px` *(int, default: `3`)*: Safety gap in pixels between the top of the taskbar and the bottom of the visualizer window so they never touch or collide.
- `fps_cap` *(int, default: `120`)*: Frame rate limiter (`60`, `120`, or `144`). Lowering to `60` reduces CPU usage even further (down to ~0.3%).
- `click_through` *(bool, default: `true`)*: Enables `WS_EX_TRANSPARENT`. Mouse clicks, right clicks, and drag-and-drop pass straight through to desktop icons and windows below.

### Audio Sensitivity & FFT Parameters

```json
"audio": {
  "sample_rate": 48000,
  "fft_size": 1024,
  "num_bands": 64,
  "attack_time_ms": 12.0,
  "decay_time_ms": 140.0,
  "sensitivity_multiplier": 1.35
}
```

- `sensitivity_multiplier` *(float, default: `1.35`)*: Global amplitude gain. Increase to `1.8`–`2.2` if you listen to quiet audio or podcasts. Decrease to `0.9`–`1.1` if you listen to heavily mastered EDM / metal.
- `attack_time_ms` *(float, default: `12.0`)*: Ballistics rise time in milliseconds. Smaller values make visualizer reaction snappier.
- `decay_time_ms` *(float, default: `140.0`)*: Ballistics fall time in milliseconds. Larger values create smoother, more relaxed decay.

### Visualizer Modes & Physics Parameters

```json
"visualizer": {
  "mode": "waveform_curve",
  "bar_spacing_px": 3.0,
  "corner_radius_px": 2.5,
  "peak_cap_enabled": true,
  "line_thickness_px": 2.5,
  "fill_under_wave": true,
  "fill_opacity": 0.30
}
```

- `mode` *(string)*:
  - `"equalizer_bars"`: Classic 64-band studio spectrum analyzer with floating peak caps.
  - `"waveform_curve"`: Silky smooth continuous Catmull-Rom Bezier curve with multi-color gradient and soft translucent aurora fill.
  - `"mirrored_wave"`: Symmetrical top-and-bottom vertical spectrum anchored along the center baseline.
- `bar_spacing_px` *(float, default: `3.0`)*: Horizontal gap between equalizer bars in pixels.
- `corner_radius_px` *(float, default: `2.5`)*: Rounded corner radius of equalizer bars.
- `peak_cap_enabled` *(bool, default: `true`)*: Enables/disables the floating peak indicators in bar mode.
- `fill_under_wave` *(bool, default: `true`)*: Enables the translucent colored fill underneath the curve in `"waveform_curve"` mode.
- `fill_opacity` *(float, default: `0.30`)*: Opacity of the curve fill (`0.0` to `1.0`).

### Color Mixer & Palette Presets

AetherWave supports custom multi-stop color gradients. Each stop has a normalized `stop` position (`0.0` = Left/Bass, `1.0` = Right/Treble) and a `hex` color code.

#### Default "Neon Spectrum" Palette:
```json
"color_palette": [
  { "stop": 0.00, "hex": "#FF1A4B", "label": "Crimson Bass" },
  { "stop": 0.30, "hex": "#FF5722", "label": "Orange Mid-Bass" },
  { "stop": 0.65, "hex": "#8B5CF6", "label": "Electric Violet Mids" },
  { "stop": 1.00, "hex": "#00A3FF", "label": "Azure Neon Treble" }
]
```

#### Preset 1: "Cyberpunk Night" (Cyan, Magenta, Gold)
```json
"color_palette": [
  { "stop": 0.00, "hex": "#FF007F", "label": "Hot Pink Bass" },
  { "stop": 0.40, "hex": "#7928CA", "label": "Deep Purple Mids" },
  { "stop": 0.80, "hex": "#00DFD8", "label": "Cyan Treble" },
  { "stop": 1.00, "hex": "#FEE140", "label": "Gold Shimmer" }
]
```

#### Preset 2: "Monochrome Ice" (Clean White & Platinum)
```json
"color_palette": [
  { "stop": 0.00, "hex": "#E0E0E0", "label": "Silver Bass" },
  { "stop": 0.50, "hex": "#FFFFFF", "label": "Pure White Vocals" },
  { "stop": 1.00, "hex": "#A0D8EF", "label": "Ice Blue Treble" }
]
```

#### Preset 3: "Blood Orange Fire"
```json
"color_palette": [
  { "stop": 0.00, "hex": "#8B0000", "label": "Dark Red Bass" },
  { "stop": 0.50, "hex": "#FF4500", "label": "Orange Fire Mids" },
  { "stop": 1.00, "hex": "#FFD700", "label": "Golden Sparks" }
]
```

---

## 3. Source Code Customization (C++20)

### Project Directory Structure
```
desktop-waveform-visualizer/
├── include/
│   ├── types.hpp              <- Data structures, enums, config structs
│   ├── dsp.hpp                <- FFT, Hann window, ballistics, Pink Noise pre-emphasis
│   ├── audio_capture.hpp      <- WASAPI loopback audio stream
│   ├── color_mixer.hpp        <- Multi-color interpolation & glow math
│   ├── renderer.hpp           <- Direct2D 1.1 GPU rendering pipeline
│   ├── window.hpp             <- Win32 transparent window, tray icon, hotkeys
│   ├── config_manager.hpp     <- JSON watcher & hot-reload parser
│   └── platform/              <- Cross-platform abstraction headers
├── src/
│   ├── main.cpp               <- WinMain entry, loop, single-instance mutex
│   ├── dsp.cpp                <- DSP implementation
│   ├── audio_capture.cpp      <- WASAPI loopback implementation
│   ├── color_mixer.cpp        <- Color blending & smoothstep
│   ├── renderer.cpp           <- Bar, Waveform Curve, & Mirrored renderers
│   ├── window.cpp             <- Win32 layered window, taskbar sync
│   ├── config_manager.cpp     <- JSON reader/writer
│   └── platform/
│       ├── linux/             <- PulseAudio loopback & X11 XShape click-through
│       └── macos/             <- CoreAudio loopback & Cocoa NSWindow overlay
├── config/
│   └── config.json            <- Default hot-reload configuration
├── build.bat                  <- 64-bit MSVC compile script
├── run.bat / stop.bat         <- Windows background launcher & killer
├── run.sh / stop.sh           <- Linux/macOS launcher & killer
└── CMakeLists.txt             <- Multi-platform CMake build configuration
```

---

### How to Tweak Audio DSP & Vocal Curves (`src/dsp.cpp`)

The acoustic balancing between bass, vocals, and treble is defined in `src/dsp.cpp` in `DspEngine::ProcessFft`.

#### Modifying Vocal Pre-Emphasis Weight:
Open `src/dsp.cpp` around line 125:
```cpp
// Pink Noise Pre-Emphasis Curve:
float weight = 1.0f;
if (freq < 150.0f) {
    weight = 1.0f + (freq / 150.0f) * 0.4f;       // Sub-bass (1.0x - 1.4x)
} else if (freq < 350.0f) {
    weight = 1.4f + ((freq - 150.0f) / 200.0f) * 2.0f; // Warmth / mid-bass (1.4x - 3.4x)
} else if (freq < 800.0f) {
    weight = 3.4f + ((freq - 350.0f) / 450.0f) * 3.6f; // Low vocal fundamental (3.4x - 7.0x)
} else if (freq < 2500.0f) {
    weight = 7.0f + ((freq - 800.0f) / 1700.0f) * 5.0f; // Core vocal presence (7.0x - 12.0x)
} else if (freq < 6000.0f) {
    weight = 12.0f + ((freq - 2500.0f) / 3500.0f) * 4.5f; // Vocal harmonics / guitars (12.0x - 16.5x)
} else {
    weight = 16.5f + ((freq - 6000.0f) / 14000.0f) * 5.5f; // Treble / air (16.5x - 22.0x)
}
```
- Want **even stronger vocals**? Increase `7.0f` and `12.0f` up to `10.0f` and `15.0f`.
- Want **heavier sub-bass**? Increase the `< 150.0f` weight to `2.0f`.

#### Modifying Decibel Scaling Range:
Around line 150 of `src/dsp.cpp`:
```cpp
const float min_db = -44.0f; // Floor threshold
const float max_db = -2.0f;  // Ceiling threshold
```
- Lowering `min_db` to `-50.0f` captures whisper-quiet sounds.
- Raising `min_db` to `-35.0f` ensures only prominent sounds trigger the visualizer.

---

### How to Customize Waveform Curve (`src/renderer.cpp`)

The curve in `VisualizerRenderer::RenderWaveformCurve` is strictly synchronized with the ballistics and speed of `Equalizer Bars`:

1. **Direct Dynamic Responsiveness:**
   ```cpp
   // Directly maps the 64 frequency bands with a light 3-tap spatial filter:
   curve_heights[i] = prev * 0.15f + curr * 0.70f + next * 0.15f;
   ```
   Rises and falls with the exact same 12ms attack and 140ms decay as Equalizer Bars.

2. **Glow Thickness & Line Weights:**
   ```cpp
   // Ambient glow width:
   m_dc_target->DrawGeometry(path, m_gradient_brush, config.wave_thickness_px + 3.0f);
   // Crisp core width:
   m_dc_target->DrawGeometry(path, m_gradient_brush, config.wave_thickness_px);
   ```

---

### How to Customize Equalizer Bars & Peak Gravity (`src/renderer.cpp`)

In `VisualizerRenderer::RenderEqualizerBars`:
- `max_bar_h`: Maximum height of bars (`static_cast<float>(m_height) - 12.0f`).
- `peak_rect`: Size of the floating peak indicator.
- Gravity acceleration is in `src/dsp.cpp`:
  ```cpp
  m_peak_velocity[i] += (config.peak_gravity * 0.001f) * dt;
  ```

---

### How to Add a Brand New Visualizer Mode (Step-by-Step)

Want to add a 4th mode (e.g., `"circular_dots"` or `"dna_helix"`)? Follow these 5 steps:

#### Step 1: Add enum in `include/types.hpp`
```cpp
enum class VisualizerMode {
    EQUALIZER_BARS = 0,
    WAVEFORM_CURVE = 1,
    MIRRORED_WAVE = 2,
    MY_NEW_MODE = 3 // <- Add this
};
```

#### Step 2: Add rendering function in `include/renderer.hpp`
```cpp
void RenderMyNewMode(const std::vector<float>& bands, float rms_energy, const AppConfig& config, const ColorMixer& color_mixer);
```

#### Step 3: Implement the rendering function in `src/renderer.cpp`
```cpp
void VisualizerRenderer::RenderMyNewMode(const std::vector<float>& bands, float rms_energy, const AppConfig& config, const ColorMixer& color_mixer) {
    // Write your Direct2D drawing code here!
    // Example: draw circles, lines, or particles using m_dc_target
}
```
And add it to the `switch (config.mode)` block in `VisualizerRenderer::Render`.

#### Step 4: Add JSON name in `src/config_manager.cpp`
In `ParseConfigJson`:
```cpp
if (mode_str == "my_new_mode") out_config.mode = VisualizerMode::MY_NEW_MODE;
```

#### Step 5: Add Tray Menu Item in `src/window.cpp`
```cpp
AppendMenuW(hMenu, MF_STRING, 104, L"Mode: My New Mode");
```

---

### How to Modify Window Click-Through & Taskbar Tracking (`src/window.cpp`)

- **Click-Through:** Handled via `WM_NCHITTEST` returning `HTTRANSPARENT`. To make the visualizer draggable, return `HTCAPTION` instead.
- **Taskbar Docking Coordinates:** Located in `WindowManager::CheckAndRepositionIfNeeded()`. It queries `FindWindowW(L"Shell_TrayWnd", NULL)` and repositions the window dynamically if the taskbar moves.

---

## 4. Compiling & Rebuilding

### Windows (MSVC)
1. Open terminal in the project directory.
2. Run the build script:
   ```cmd
   build.bat
   ```
   *(The script automatically stops running instances, calls `cl.exe` with C++20 flags `/std:c++20`, `/O2`, and links `user32.lib gdi32.lib shell32.lib ole32.lib d2d1.lib`, outputting `bin\AetherWave.exe`)*.
3. Launch with `run.bat`.

### Linux (Ubuntu/Debian/Arch/Fedora)
1. Install development dependencies (if not already installed):
   ```bash
   sudo apt update && sudo apt install -y cmake g++ libpulse-dev libcairo2-dev libx11-dev libxext-dev
   ```
2. Simply run the launcher:
   ```bash
   bash run.sh
   # or: chmod +x run.sh stop.sh && ./run.sh
   ```
   *(On first run, `run.sh` automatically detects if the binary is missing, compiles it with CMake using all CPU cores, and launches AetherWave in the background. Subsequent runs start instantly!)*  
   > ⚠️ **DO NOT run with `sudo`!** Desktop graphical and audio applications must run as your normal user account so they can connect to your X11 display and PulseAudio/PipeWire audio server.

### macOS (Apple Silicon & Intel)
1. Ensure Xcode Command Line Tools are installed:
   ```bash
   xcode-select --install
   ```
2. Simply run the launcher:
   ```bash
   ./run.sh
   ```
   *(Auto-compiles and generates `build/AetherWave.app` on first run! You can also double-click `build/AetherWave.app` directly in Finder or run `./bundle_macos.sh`)*.

---

## 5. Author & Credits

- **Creator & Lead Architect:** **Amir (Only-One-Kind)**
- **Role:** Full system architecture, DSP pre-emphasis curve design, Direct2D render design, multi-color gradient mixing specifications, single-instance POSIX/Win32 engine, and cross-platform planning.
- **License:** Open for personal and community enhancement. Credit to Amir (Only-One-Kind) must be retained in all forks, derivatives, and documentation.
