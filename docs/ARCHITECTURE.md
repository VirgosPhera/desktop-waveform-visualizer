# AetherWave — System Architecture & Engineering Blueprint

> **Creator & Lead Architect:** Amir (Only-One-Kind)  
> **System Name:** AetherWave  
> **Version:** 1.1.0  
> **Repository:** `desktop-waveform-visualizer`

---

## 1. High-Level Architecture Diagram

```
+-----------------------------------------------------------------------------+
|                                AUDIO HARDWARE                               |
|        (Speakers / Headphones / Realtek / USB DAC / Bluetooth)               |
+-----------------------------------------------------------------------------+
                                      |
                                      v (Real-time playback output)
+-----------------------------------------------------------------------------+
|                           AUDIO CAPTURE SUBSYSTEM                           |
|  - Windows: WASAPI Loopback (IAudioClient / IAudioCaptureClient)            |
|  - Linux:   PulseAudio Simple API (@DEFAULT_SINK@.monitor)                  |
|  - macOS:   CoreAudio / AudioQueue / ScreenCaptureKit                       |
|  * Thread: Dedicated Realtime Capture Thread                                |
|  * Output: Lock-free Ring Buffer (32-bit IEEE float @ 44.1/48kHz Stereo)    |
+-----------------------------------------------------------------------------+
                                      |
                                      v (Lock-free sample extraction)
+-----------------------------------------------------------------------------+
|                             DSP & FFT PIPELINE                              |
|  1. Hann Windowing: w(n) = 0.5 * (1 - cos(2*pi*n / (N - 1)))                |
|  2. Cooley-Tukey Radix-2 FFT (1024 points) -> Complex Frequency Bins        |
|  3. Spectral Power Spectrum: |X[k]| = sqrt(Re^2 + Im^2) / N                 |
|  4. Logarithmic Band Aggregation: 64 Log Bands (20 Hz - 20,000 Hz)          |
|  5. Pink Noise Pre-Emphasis Compensation:                                   |
|     * Bass: 1.0x - 1.4x                                                     |
|     * Vocals (300Hz - 6000Hz): +4.5x up to +12.0x (Organic Vocal Movement)  |
|     * Treble (6kHz - 20kHz): +16.5x up to +22.0x                           |
|  6. Logarithmic Decibel Compression: dB = 20 * log10(magnitude) [-44..-2dB] |
|  7. Ballistics Filter: Attack 12ms (EMA) / Decay 140ms (EMA)                |
|  8. Gravity Peak Physics: Velocity += g * dt; PeakY -= Velocity * dt        |
+-----------------------------------------------------------------------------+
                                      |
                                      v (Smoothed 64 Bands + RMS Energy)
+-----------------------------------------------------------------------------+
|                        COLOR MIXER & SHADER ENGINE                          |
|  * Multi-Stop Palette Interpolation (Smoothstep S(t) = 3t^2 - 2t^3)         |
|  * Stops: Crimson Bass (#FF1A4B) -> Orange (#FF5722) ->                     |
|           Electric Violet Vocals (#8B5CF6) -> Azure Treble (#00A3FF)        |
|  * Dynamic Amplitude Glow Boost: Modulates luminance/saturation by RMS     |
|  * ID2D1LinearGradientBrush (Hardware Accelerated GPU Linear Gradient)     |
+-----------------------------------------------------------------------------+
                                      |
                                      v
+-----------------------------------------------------------------------------+
|                             RENDERING ENGINE                                |
|  - Windows: Direct2D 1.1 + DirectWrite GPU Pipeline                         |
|  - Linux:   X11 + Cairo 2D Vector Graphics                                  |
|  - macOS:   Cocoa CoreGraphics                                              |
|                                                                             |
|  * MODE 1: EQUALIZER BARS                                                   |
|    - 64 Rounded Rectangles with Floating Peak Cap indicators                |
|  * MODE 2: WAVEFORM CURVE                                                   |
|    - 5-Tap Spatial Gaussian Smoothing                                       |
|    - Smoothstep Edge Tapering to Screen Edges                               |
|    - Liquid Inertia Temporal Ballistics (Attack 0.32 / Decay 0.14)          |
|    - Catmull-Rom to Cubic Bezier Splines (Continuous C1 curvature)          |
|    - Dual Stroke (Ambient Glow + Sharp Core) + Aurora Gradient Fill         |
|  * MODE 3: MIRRORED WAVE                                                    |
|    - Vertical Symmetrical Spectrum radiating from center baseline           |
+-----------------------------------------------------------------------------+
                                      |
                                      v (BitBlt / UpdateLayeredWindow)
+-----------------------------------------------------------------------------+
|                        WINDOW & DESKTOP DOCKING HOST                        |
|  - Attributes: WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE         |
|  - Zero Taskbar Collision: Dynamic query FindWindowW(L"Shell_TrayWnd")      |
|    Docks Top = TaskbarTop - Height - 2px gap (auto-repositioned every 0.5s) |
|  - Single-Instance Enforcement: Named Mutex (Win32) / flock lockfile (POSIX)|
|  - Hotkey Listener: Ctrl + Shift + V                                        |
|  - Notification Tray Menu: Exit, Switch Modes, Toggle Visibility            |
|  - Hot-Reload Watcher: ReadDirectoryChangesW (config.json)                  |
+-----------------------------------------------------------------------------+
```

---

## 2. Component Decomposition & Source Tree

| Module | Files | Responsibility |
| :--- | :--- | :--- |
| **Main Host** | `src/main.cpp` | Application entry point, high-resolution frame limiter (120 FPS), Single-Instance Mutex, subsystem lifecycle. |
| **Audio Capture** | `src/audio_capture.cpp`, `include/audio_capture.hpp` | WASAPI loopback audio capture thread, format negotiation (48kHz Float32), lock-free ring buffer writing. |
| **DSP Engine** | `src/dsp.cpp`, `include/dsp.hpp` | In-place Cooley-Tukey Radix-2 FFT (1024 pts), Hann window, 64 logarithmic bands, Pink Noise pre-emphasis tilt, attack/decay ballistics, gravity peak caps. |
| **Color Engine** | `src/color_mixer.cpp`, `include/color_mixer.hpp` | Multi-stop hex color parsing, smoothstep interpolation, dynamic amplitude glow modulation. |
| **Renderer** | `src/renderer.cpp`, `include/renderer.hpp` | Direct2D 1.1 hardware accelerated render target, Linear Gradient Brushes, Bezier path geometry generator, layered window blitting. |
| **Window Manager** | `src/window.cpp`, `include/window.hpp` | Win32 borderless layered click-through window, DPI calculation, Taskbar dynamic positioning sync, system tray icon, `Ctrl+Shift+V` hotkey. |
| **Config Manager** | `src/config_manager.cpp`, `include/config_manager.hpp` | JSON parser, hot-reload background directory watcher (`ReadDirectoryChangesW`). |
| **Platform Linux** | `src/platform/linux/` | PulseAudio loopback audio capture + X11 XShape transparent layered window. |
| **Platform macOS** | `src/platform/macos/` | CoreAudio loopback capture + Cocoa borderless `NSWindow` with `ignoresMouseEvents`. |

---

## 3. Mathematical Foundations

### Fast Fourier Transform (FFT)
For an $N$-point time-domain sequence $x[n]$ with $N = 1024$:
$$X[k] = \sum_{n=0}^{N-1} x[n] \cdot w[n] \cdot e^{-j 2\pi k n / N}$$

Where $w[n]$ is the **Hann Window function** to minimize spectral leakage:
$$w[n] = 0.5 \cdot \left(1 - \cos\left(\frac{2\pi n}{N - 1}\right)\right)$$

### Acoustic Pre-Emphasis (Vocal & Melodic Equalization)
Real-world musical recordings naturally follow a **Pink Noise spectral slope** ($\approx -3\text{ to }-4.5\text{ dB/octave}$). A flat FFT causes bass kick drums (50Hz–100Hz) to possess 20x–30x the linear amplitude of vocal harmonics (800Hz–4000Hz).

AetherWave applies a piecewise compensatory curve $W(f)$ before decibel conversion:
- $20\text{Hz} \le f < 150\text{Hz}$ (Sub-Bass): $W(f) = 1.0 + 0.4 \cdot (f / 150)$
- $150\text{Hz} \le f < 350\text{Hz}$ (Mid-Bass): $W(f) = 1.4 + 2.0 \cdot ((f - 150) / 200)$
- $350\text{Hz} \le f < 800\text{Hz}$ (Low Vocals): $W(f) = 3.4 + 3.6 \cdot ((f - 350) / 450)$
- $800\text{Hz} \le f < 2500\text{Hz}$ (Core Vocals): $W(f) = 7.0 + 5.0 \cdot ((f - 800) / 1700)$
- $2500\text{Hz} \le f < 6000\text{Hz}$ (Presence & Harmonics): $W(f) = 12.0 + 4.5 \cdot ((f - 2500) / 3500)$
- $6000\text{Hz} \le f \le 20000\text{Hz}$ (Treble & Air): $W(f) = 16.5 + 5.5 \cdot ((f - 6000) / 14000)$

### Ballistics Dampening (Exponential Moving Average)
For frame delta time $\Delta t$ and target band energy $E_{\text{raw}}$:
$$\alpha = \begin{cases} 
1 - e^{-\Delta t / \tau_{\text{attack}}} & \text{if } E_{\text{raw}} > E_{\text{smooth}} \quad (\tau_{\text{attack}} = 12\text{ms}) \\
1 - e^{-\Delta t / \tau_{\text{decay}}} & \text{if } E_{\text{raw}} \le E_{\text{smooth}} \quad (\tau_{\text{decay}} = 140\text{ms})
\end{cases}$$
$$E_{\text{smooth}}(t) = E_{\text{smooth}}(t - \Delta t) + \alpha \cdot \left(E_{\text{raw}} - E_{\text{smooth}}(t - \Delta t)\right)$$

### Catmull-Rom to Cubic Bezier Conversion
Given consecutive points $P_0, P_1, P_2, P_3$, the cubic Bezier control points for the segment between $P_1$ and $P_2$ are:
$$CP_1 = P_1 + \frac{P_2 - P_0}{6}$$
$$CP_2 = P_2 - \frac{P_3 - P_1}{6}$$
Ensuring $C^1$ continuity (continuous slope and curvature) across all 64 frequency control points.

---

## 4. Single-Instance Concurrency Guarantee

To prevent dual-running or overlapping windows:
- **Windows:** Calls `CreateMutexW(NULL, TRUE, L"Local\\AetherWave_SingleInstance_Mutex")`. If `GetLastError() == ERROR_ALREADY_EXISTS`, the secondary process un-hides the existing window and terminates immediately in `<50ms`.
- **Linux & macOS:** Opens lockfile `$XDG_RUNTIME_DIR/aetherwave.lock` and acquires a non-blocking kernel lock via `flock(fd, LOCK_EX | LOCK_NB)`. If locked, process exits with status 0.

---

## 5. Credits & Attribution

- **Lead Architect & Original Author:** **Amir (Only-One-Kind)**
- **Design Philosophy:** Zero-slop, Material 3 minimalism, dark mode purity, ultra-low resource footprint.
