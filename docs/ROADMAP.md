# ROADMAP — Feature Hierarchy & Milestone Matrix

> **Lead Architect & Original Author:** Amir (Only-One-Kind)  
> **System Name:** AetherWave  
> **Version:** 1.1.0  
> **Repository:** `desktop-waveform-visualizer`

---

## Executive Feature Summary

| ID | Feature Module | Core Benefit | Status | Sub-Features |
| :--- | :--- | :--- | :--- | :--- |
| **F-01** | **Audio Capture** | Real-time zero-cable loopback audio recording | Complete (Windows, Linux, macOS) | 3 Sub-features |
| **F-02** | **DSP & Equal-Loudness** | 1024-pt FFT, vocal pre-emphasis, ballistics | Complete | 4 Sub-features |
| **F-03** | **Display & Docking** | Bottom-screen docking, zero-overlap, click-through | Complete | 4 Sub-features |
| **F-04** | **Visualizer Modes** | Equalizer Bars, Silky Waveform Curve, Mirrored Wave | Complete | 3 Sub-features |
| **F-05** | **Color Mixer** | Multi-color smoothstep gradient & dynamic glow | Complete | 3 Sub-features |
| **F-06** | **System & Control** | Single-Instance Mutex, Hot-Reload, Hotkeys, Tray | Complete | 3 Sub-features |

---

## Detailed Feature Hierarchy

### 1. Audio Capture (F-01 — High Priority)
- **1.1 Automatic Device Enumeration:** Detects and binds default active audio devices across speaker, headphone, and USB DAC switching.
- **1.2 Passive Zero-Cable Loopback:** WASAPI Loopback (Windows), PulseAudio Monitor (Linux), and CoreAudio (macOS) with <10ms latency.
- **1.3 Lock-Free Circular Ring Buffer:** Atomic-indexed SPSC ring buffer for glitch-free sample transport.

### 2. DSP Engine (F-02 — High Priority)
- **2.1 FFT Spectral Analysis:** 1024-point Radix-2 Cooley-Tukey FFT with Hann windowing for spectral leak reduction.
- **2.2 Logarithmic Frequency Bins:** 64-band grouping mapping 20 Hz to 20 kHz with human hearing logarithmic distribution.
- **2.3 Acoustic Pre-Emphasis Vocal Curve:** Boosts vocal frequencies (300Hz–6kHz) by +4.5x up to +12x to balance physical Pink Noise recording slopes.
- **2.4 Ballistics & Gravity Peaks:** 12ms attack / 140ms decay exponential smoothing + simulated gravitational fall for peak caps.

### 3. Display & Docking (F-03 — High Priority)
- **3.1 Dynamic Bottom-Screen Docking:** Dynamically docks strictly 2–3 px above the Windows Taskbar, Linux panel, or macOS Dock.
- **3.2 100% Click-Through Transparency:** Passes all mouse events (clicks, dragging, right-clicks) through to windows below.
- **3.3 Strict 1.5–6.0 cm Height Limiter:** Physical centimeter-to-pixel scaling respecting Per-Monitor V2 DPI.
- **3.4 Single-Instance Concurrency Enforcement:** Win32 Named Mutex and POSIX `flock` guaranteeing only 1 process runs at a time.

### 4. Visualizer Modes (F-04 — High Priority)
- **4.1 Equalizer Bars:** 64 rounded-rectangle spectrum bars with floating peak caps.
- **4.2 Waveform Curve (Ultra-Smooth):** 5-tap Gaussian spatial smoothing, smoothstep edge tapering, liquid inertia ballistics, monotonic Catmull-Rom Bezier splines, and translucent aurora fill.
- **4.3 Mirrored Stereo Wave:** Symmetrical vertical spectrum bars radiating upward and downward from the center horizontal axis.

### 5. Color Mixer (F-05 — High Priority)
- **5.1 Multi-Stop Palette Interpolation:** Smoothstep blending across Bass Red (`#FF1A4B`), Mid Orange (`#FF5722`), Vocal Violet (`#8B5CF6`), and Treble Blue (`#00A3FF`).
- **5.2 Hardware Accelerated Linear Gradient:** `ID2D1LinearGradientBrush` rendering across full monitor width.
- **5.3 Dynamic Amplitude Glow Boost:** Modulates luminance and saturation (+45%) during energetic bass drops and vocal climaxes.

### 6. System & Control (F-06 — Medium Priority)
- **6.1 JSON Hot-Reload Watcher:** Live directory monitoring via `ReadDirectoryChangesW` for instantaneous reconfiguration without restarts.
- **6.2 Global Hotkey & System Tray:** `Ctrl + Shift + V` visibility toggle and tray menu for mode switching and application exit.
- **6.3 Multi-Platform Launchers:** `run.bat`/`stop.bat` (Windows), `run.sh`/`stop.sh` (Linux & macOS), `bundle_macos.sh` (`.app` bundle), and `aetherwave.desktop`.

---

## Author & Attribution
- **Creator & Lead Architect:** **Amir (Only-One-Kind)**
- **System:** AetherWave Engine v1.1.0
