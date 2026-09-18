# AetherWave — Bottom-Screen Desktop Audio Visualizer

> **Creator & Lead Architect:** **Amir (Only-One-Kind)**  
> **Version:** 1.1.0  
> **Target Platforms:** Windows 10/11, Linux (PulseAudio/PipeWire), macOS (Apple Silicon & Intel)

A lightweight (<1% CPU, ~30-43MB RAM) and ultra-low latency (<10ms) cross-platform desktop audio visualizer for **Windows**, **Linux**, and **macOS**. Specifically engineered to run ambiently alongside music playback from YouTube Music (Pear-Desktop), Spotify, web browsers, games, or any media player.

---

## 📖 Complete Documentation Suite (`docs/`)

Looking to tweak, customize, mod, or extend AetherWave manually? Full developer documentation is available in the **[`docs/`](./docs/)** directory:

- 🛠️ **[Manual Editing & Customization Guide](./docs/MANUAL_EDITING_GUIDE.md)** — Step-by-step instructions for editing colors, DSP, vocal curves, smoothing, and adding new modes without/with compiling.
- 📐 **[System Architecture & Math Blueprint](./docs/ARCHITECTURE.md)** — High-level diagrams, FFT & Pink Noise vocal pre-emphasis formulas, Direct2D GPU pipeline, and single-instance concurrency.
- 📋 **[Product Requirements Document (PRD)](./docs/PRD.md)** — Complete FR-001 to FR-012 functional & non-functional requirements.
- 🔬 **[Technical Specifications (SPECS)](./docs/SPECS.md)** — In-depth math, ballistics, DPI, and platform API specifications.
- 🗺️ **[Feature Roadmap (ROADMAP)](./docs/ROADMAP.md)** — Feature tree and module hierarchy.
- 🎨 **[Design Tokens & Specs (DESIGN)](./docs/DESIGN.md)** — Acoustic color mapping and component specifications.

---

## 🇬🇧 English Version

### Key Features

1. **Bottom-Screen Docking without Taskbar / Panel Overlap:**
   - **Windows:** Docks strictly above the Windows Taskbar with a dynamic 2–3 px safety gap. Features live taskbar tracking (automatically re-checks every 0.5s to support auto-hiding taskbars).
   - **Linux:** Automatically queries `_NET_WORKAREA` to dock above bottom panels (KDE Plasma, Cinnamon, XFCE).
   - **macOS:** Respects `visibleFrame` to dock neatly above the macOS Dock.
   - Adjustable physical height (default 2.8 cm / ~106 px, range 1.5 cm – 6.0 cm). Automatically spans 100% of monitor width.

2. **Equal-Loudness DSP & Balanced Vocal Dynamics:**
   - Employs an acoustic *Pink Noise Pre-Emphasis* curve (+4.5x up to +12x across 300Hz–6kHz).
   - Human vocals, piano, guitars, and cymbals dance vigorously alongside punchy bass kicks.
   - Logarithmic Decibel (dB) dynamic range compression (-44 dB to -2 dB) for organic audio dynamics.

3. **100% Click-Through Transparency:**
   - Configured with native click-through attributes (`WS_EX_TRANSPARENT` on Windows, `XShape` on Linux, `ignoresMouseEvents` on macOS).
   - Mouse clicks, dragging, and desktop interactions pass completely through the visualizer without obstruction.

4. **Multi-Color Blending Engine:**
   - Smooth multi-stop color palette interpolation (Bass Red `#FF1A4B`, Mid-Bass Orange `#FF5722`, Vocal Purple `#8B5CF6`, Treble Blue `#00A3FF`).
   - Clean smoothstep transitions with zero muddy/gray artifacts.
   - Dynamic Amplitude Glow: boosts luminance and saturation (+40%) during heavy bass drops or high RMS volume.

5. **3 Visualization Modes:**
   - **Equalizer Bars:** 64 rounded-rectangle spectrum bars with gravity-driven *Floating Peak Caps*.
   - **Waveform Curve:** Smooth continuous sinusoidal curve powered by Catmull-Rom Bezier splines with semi-transparent fill.
   - **Mirrored Wave:** Vertical symmetrical stereo waveform radiating outward from the center baseline.

6. **Single-Instance Enforcement:**
   - **Windows:** Win32 Named Mutex (`Local\AetherWave_SingleInstance_Mutex`).
   - **Linux & macOS:** POSIX kernel file lock (`flock` on `$XDG_RUNTIME_DIR/aetherwave.lock`).
   - Prevents duplicate processes from launching simultaneously.

7. **Instant Controls & Hot-Reload:**
   - **Global Hotkey (Windows):** Press `Ctrl + Shift + V` anytime to instantly toggle visualizer visibility.
   - **System Tray Menu:** Right-click the notification tray icon to switch visualization modes or exit.
   - **Hot-Reload JSON:** Modify `config/config.json` in any text editor and save — changes apply live within milliseconds without restarting.

### How to Run

#### 🪟 Windows (11 / 10)
- **Start:** Double-click `run.bat` (or `bin\AetherWave.exe`).
- **Stop:** Double-click `stop.bat` or right-click the system tray icon -> "Exit AetherWave".
- **Recompile:** Run `build.bat` (uses MSVC C++20).

#### 🐧 Linux (Ubuntu / Debian / Arch / Fedora)
- **Start:** Run `bash run.sh` (or `chmod +x run.sh && ./run.sh`). *(Auto-compiles automatically on first run, then starts in background)*.
  > ⚠️ **IMPORTANT:** Do **NOT** use `sudo`! Running as root prevents access to your desktop's X11 display and PulseAudio session.
- **Stop:** Run `bash stop.sh` (or `./stop.sh`).
- **Install Dependencies (Ubuntu/Debian):**
  ```bash
  sudo apt update && sudo apt install -y cmake g++ libpulse-dev libcairo2-dev libx11-dev libxext-dev
  ```
- **1-Click Icon:** Copy `aetherwave.desktop` to `~/.local/share/applications/` or your Desktop.

#### 🍏 macOS (Apple Silicon M1/M2/M3 & Intel)
- **Start:** Just run `./run.sh` or double-click `build/AetherWave.app`! *(Auto-compiles and creates .app bundle on first run)*.
- **Stop:** Run `./stop.sh`.
- **Create Standalone App Bundle:** Run `./bundle_macos.sh`.

### Configuration (`config/config.json`)

```json
{
  "window": {
    "height_cm": 2.8,              // Physical height in centimeters (1.5 - 6.0)
    "dock_above_taskbar": true,    // Dock above taskbar / panel
    "taskbar_margin_px": 3,        // Margin above taskbar in pixels
    "fps_cap": 120,                // Frame rate limiter (60 / 120 / 144)
    "click_through": true          // 100% mouse click-through
  },
  "audio": {
    "sensitivity_multiplier": 1.35 // Visual amplitude sensitivity
  },
  "visualizer": {
    "mode": "equalizer_bars",      // "equalizer_bars", "waveform_curve", "mirrored_wave"
    "bar_spacing_px": 3.0,         // Gap between bars
    "peak_cap_enabled": true       // Floating peak indicators
  },
  "color_mixer": {
    "color_palette": [
      { "stop": 0.00, "hex": "#FF1A4B", "label": "Crimson Bass" },
      { "stop": 0.30, "hex": "#FF5722", "label": "Orange Mid-Bass" },
      { "stop": 0.65, "hex": "#8B5CF6", "label": "Electric Violet Mids" },
      { "stop": 1.00, "hex": "#00A3FF", "label": "Azure Neon Treble" }
    ],
    "amplitude_glow_boost": 0.45
  }
}
```

---

## 🇮🇩 Versi Bahasa Indonesia

### Fitur Utama

1. **Docking Dasar Layar Tanpa Nindih Taskbar:**
   - **Windows:** Menempel presisi tepat di atas Windows Taskbar dengan celah pengaman 2–3 px agar tidak menutupi icon taskbar. Dilengkapi pemantauan dinamis posisi taskbar (auto-reposition setiap 0.5s saat taskbar auto-hide atau aktif).
   - **Linux:** Otomatis membaca `_NET_WORKAREA` agar menempel di atas panel bawah (KDE Plasma, Cinnamon, XFCE).
   - **macOS:** Membaca `visibleFrame` agar selalu berada tepat di atas macOS Dock.
   - Tinggi fisik visualizer dapat diatur bebas (default 2.8 cm / ~106 px, rentang 1.5 cm – 6.0 cm). Bentang lebar otomatis 100% monitor aktif.

2. **DSP Equal-Loudness & Respon Vokal Dinamis:**
   - Menerapkan kurva kompensasi *Pink Noise Pre-Emphasis* (+4.5x hingga +12x pada rentang 300Hz–6kHz).
   - Vokal penyanyi, piano, melodi gitar, dan simbal berdenyut lincah seimbang bersamaan dengan hentakan bass kick drum.
   - Konversi skala Decibel (dB) terkompresi rentang -44 dB hingga -2 dB untuk dinamika audio organik.

3. **100% Click-Through Transparency:**
   - Dilengkapi atribut Win32 `WS_EX_TRANSPARENT`, Linux `XShape`, dan macOS `ignoresMouseEvents`.
   - Klik mouse, drag file, dan interaksi desktop/taskbar tembus 100% tanpa hambatan.

4. **Multi-Color Mixing Engine:**
   - Pencampuran spektrum warna cerdas (Bass Merah `#FF1A4B`, Mid-Bass Oranye `#FF5722`, Vokal Ungu `#8B5CF6`, Treble Biru `#00A3FF`).
   - Smoothstep interpolation tanpa artefak warna keruh/abu-abu.
   - Dynamic Amplitude Glow: pendaran warna otomatis berkilau lebih terang (+40% luminance) saat dentuman bass keras.

5. **3 Mode Visualisasi:**
   - **Equalizer Bars:** 64 bar rounded-corner dengan *Floating Peak Caps* (titik puncak melayang yang jatuh dengan gravitasi).
   - **Waveform Curve:** Gelombang continuous sinusoidal halus berbasis Catmull-Rom Bezier spline dengan isian warna transparan.
   - **Mirrored Wave:** Gelombang audio stereo simetris atas-bawah dari poros tengah.

6. **Proteksi Anti-Double Program (Single-Instance):**
   - Mengunci Named Mutex di Windows dan `flock` kernel di Linux/macOS sehingga mustahil program berjalan dobel.

7. **Kontrol Cepat & Hot-Reload:**
   - **Hotkey Global (Windows):** Tekan `Ctrl + Shift + V` untuk menyembunyikan/menampilkan visualizer secara instan.
   - **System Tray:** Klik kanan icon di area notifikasi Windows taskbar untuk mengganti mode atau keluar.
   - **Hot-Reload JSON:** Edit `config/config.json` (warna, tinggi, sensitivitas), perubahan langsung aktif seketika tanpa restart aplikasi.

### Cara Menjalankan

#### 🪟 Windows (11 / 10)
- **Menjalankan:** Double-click `run.bat` (atau `bin\AetherWave.exe`).
- **Menghentikan:** Double-click `stop.bat` atau klik kanan icon tray -> "Exit AetherWave".
- **Compile Ulang:** Jalankan `build.bat` (menggunakan MSVC C++20).

#### 🐧 Linux (Ubuntu / Debian / Arch / Fedora)
- **Menjalankan:** Cukup jalankan `bash run.sh` (atau `chmod +x run.sh && ./run.sh`). *(Otomatis meng-compile pada run pertama jika binary belum ada, lalu langsung jalan di background)*.
  > ⚠️ **PENTING:** **JANGAN** gunakan `sudo`! Menjalankan dengan sudo/root akan memblokir akses ke display X11 dan sesi audio PulseAudio desktop kamu.
- **Menghentikan:** Jalankan `bash stop.sh` (atau `./stop.sh`).
- **Install Dependensi (Ubuntu/Debian jika dibutuhkan):**
  ```bash
  sudo apt update && sudo apt install -y cmake g++ libpulse-dev libcairo2-dev libx11-dev libxext-dev
  ```
- **Shortcut 1-Klik:** Copy `aetherwave.desktop` ke `~/.local/share/applications/` atau taruh di Desktop.

#### 🍏 macOS (Apple Silicon & Intel)
- **Menjalankan:** Cukup jalankan `./run.sh` atau double-click `build/AetherWave.app`! *(Otomatis compile dan generate app bundle pada run pertama)*.
- **Menghentikan:** Jalankan `./stop.sh`.
- **Buat .app Bundle Terpisah:** Jalankan `./bundle_macos.sh`.

### Kustomisasi `config/config.json`

```json
{
  "window": {
    "height_cm": 2.8,              // Tinggi visualizer dalam centimeter (1.5 - 6.0)
    "dock_above_taskbar": true,    // Menempel di atas taskbar / panel
    "taskbar_margin_px": 3,        // Celah pengaman di atas taskbar
    "fps_cap": 120,                // Batas FPS (60 / 120 / 144)
    "click_through": true          // Tembus klik mouse 100%
  },
  "audio": {
    "sensitivity_multiplier": 1.35 // Sensitivitas respons goyangan
  },
  "visualizer": {
    "mode": "equalizer_bars",      // "equalizer_bars", "waveform_curve", "mirrored_wave"
    "bar_spacing_px": 3.0,         // Jarak antar tiang bar
    "peak_cap_enabled": true       // Indikator puncak melayang
  },
  "color_mixer": {
    "color_palette": [
      { "stop": 0.00, "hex": "#FF1A4B", "label": "Crimson Bass" },
      { "stop": 0.30, "hex": "#FF5722", "label": "Orange Mid-Bass" },
      { "stop": 0.65, "hex": "#8B5CF6", "label": "Electric Violet Mids" },
      { "stop": 1.00, "hex": "#00A3FF", "label": "Azure Neon Treble" }
    ],
    "amplitude_glow_boost": 0.45
  }
}
```

---

## 👑 Author & Credits / Pembuat & Kredit

- **Lead Architect & Original Author / Pembuat Utama:** **Amir (Only-One-Kind)**
- **Role:** System Architecture, Equal-Loudness DSP Engine, Pink Noise Pre-Emphasis Curve Design, Direct2D Render Pipeline, Multi-Color Blending Math, Single-Instance Mutex Engine, and Cross-Platform Engineering.
- **License & Rights:** Open for personal enhancement and community modding. Attribution and credit to **Amir (Only-One-Kind)** must be preserved in all forks, distributions, and modified versions.
