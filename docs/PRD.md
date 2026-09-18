# PRD — Project Requirements Document
# AetherWave: Cross-Platform Desktop Bottom-Screen Waveform & Equalizer Visualizer

> **Lead Architect & Original Author:** Amir (Only-One-Kind)  
> **System Name:** AetherWave  
> **Version:** 1.1.0  
> **Repository:** `desktop-waveform-visualizer`

---

## Overview
AetherWave adalah aplikasi visualizer audio desktop untuk Windows 11/10, Linux (Ubuntu/Debian/Arch/Fedora), dan macOS (Apple Silicon & Intel) dengan performa tinggi, beroperasi sebagai overlay transparan di bagian paling bawah layar (bottom of screen / di atas taskbar) dengan dimensi ringkas (tinggi terbatas 1.5–6.0 cm / ~60–230 px) tanpa mengganggu produktivitas atau memakan ruang kerja pengguna. Visualizer ini menangkap aliran audio sistem secara real-time (WASAPI Loopback di Windows, PulseAudio/PipeWire di Linux, CoreAudio di macOS) dari sumber pemutaran apa pun (YouTube Music, Pear-Desktop, Spotify, browser, game, media player) tanpa memerlukan virtual audio cable atau konfigurasi stereo mix manual.

Aplikasi menghadirkan tiga mode visualisasi:
1. **Equalizer Bars:** Bar Spectrum Analyzer 64-band dengan *Floating Peak Caps* bergaya equalizer studio.
2. **Waveform Curve:** Gelombang continuous ultra-halus (*silky smooth*) berbasis Catmull-Rom Bezier spline, 5-tap Gaussian spatial smoothing, smoothstep edge tapering, dan fill aurora transparan.
3. **Mirrored Wave:** Gelombang audio stereo simetris atas-bawah dari poros horizontal tengah.

Fitur pembeda utama adalah sistem **Multi-Color Blending Engine** yang memungkinkan pencampuran n-warna (Bass Merah `#FF1A4B`, Mid-Bass Oranye `#FF5722`, Vokal Ungu `#8B5CF6`, Treble Biru `#00A3FF`) dengan modulasi pendaran cahaya (*Dynamic Amplitude Glow Boost*) secara real-time. Jendela aplikasi bersifat 100% click-through (tembus klik mouse), bebas border/titlebar, dan dirancang dengan konsumsi daya ultra-ringan (<1% CPU, ~30–43MB RAM pada 120 FPS).

---

## Requirements

### Functional Requirements (FR)
- **FR-001 (Zero-Cable Audio Loopback Capture):** Menangkap audio output stereo 32-bit float secara otomatis dari default audio output endpoint tanpa kabel virtual.
  - Windows: WASAPI Loopback (`IAudioClient`, `IAudioCaptureClient`).
  - Linux: PulseAudio Simple API monitor stream (`@DEFAULT_SINK@.monitor`).
  - macOS: CoreAudio / AudioQueue.
- **FR-002 (DSP & FFT Engine):** Melakukan Fast Fourier Transform (FFT) real-time ukuran window 1024 titik, menerapkan Hann window function untuk mereduksi spectral leakage, dan membagi frekuensi ke dalam 64 band logaritmik (rentang 20 Hz – 20.000 Hz).
- **FR-003 (Vocal & Melodic Pre-Emphasis):** Mengkompensasi kemiringan akustik *Pink Noise* dengan bobot kurva penguatan:
  - Sub-bass (20-150Hz): 1.0x - 1.4x
  - Vokal & instrumen utama (300Hz - 6000Hz): +4.5x s.d. +12.0x
  - Treble (6kHz - 20kHz): +16.5x s.d. +22.0x
  Memastikan artikulasi vokal penyanyi bergerak lincah dan tidak tenggelam oleh ketukan drum/bass.
- **FR-004 (Triple Visualizer Modes):** Mendukung pergantian instan antara Equalizer Bars, Waveform Curve, dan Mirrored Wave.
- **FR-005 (Strict Dimension & Height Cap):** Tinggi visualizer dibatasi secara ketat pada rentang 1.5–6.0 cm (default 2.8 cm / ~106 px pada 96 DPI), dengan lebar otomatis 100% bentang monitor aktif.
- **FR-006 (Zero Taskbar / Panel Collision):** Menempel presisi tepat 2–3 px di atas Windows Taskbar (`Shell_TrayWnd`), panel Linux (`_NET_WORKAREA`), atau macOS Dock (`visibleFrame`). Dilengkapi pemantauan dinamis posisi taskbar tiap 0.5s.
- **FR-007 (Dynamic Multi-Color Mixer):** Mendukung pencampuran palet warna bergradasi mulus (Bass Merah, Mid-Bass Oranye, Vokal Ungu, Treble Biru) dengan modulasi pendaran cahaya (*Glow Boost*) berbasis energi RMS lagu.
- **FR-008 (Click-Through & Input Transparency):** Jendela visualizer wajib memiliki atribut window `WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE` di Windows, `XShape` di Linux, dan `ignoresMouseEvents` di macOS, memastikan klik mouse tembus 100%.
- **FR-009 (Decay Physics & Liquid Ballistics):** Pengaturan smoothing audio menggunakan Exponential Moving Average (EMA) dengan parameter Attack cepat (12ms) dan Decay halus (140ms), serta inersia cairan asimetris pada mode kurva gelombang.
- **FR-010 (Single-Instance Concurrency Enforcement):** Mengunci Named Mutex di Windows dan POSIX `flock` di Linux/macOS sehingga mustahil program berjalan dobel.
- **FR-011 (Hot-Reload Configuration):** Konfigurasi disimpan dalam format JSON (`config.json`), mendukung automatic file-watcher reload tanpa perlu restart aplikasi.
- **FR-012 (System Tray & Global Hotkey):** Menyediakan icon di system tray untuk akses cepat (ganti mode, toggle visibility, exit) serta hotkey global `Ctrl + Shift + V`.

---

## Architecture Flow

```
[ Audio Endpoint (WASAPI / PulseAudio / CoreAudio) ]
                        │ (Passive Loopback Capture Thread)
                        ▼
          [ Audio Ring Buffer (Lock-Free) ]
                        │
                        ▼
      [ DSP / FFT Processing Pipeline (Thread Worker) ]
        ├─ Hann Windowing (1024 pts)
        ├─ Cooley-Tukey Radix-2 FFT Computation
        ├─ 64 Logarithmic Frequency Bands (20 Hz - 20 kHz)
        ├─ Pink Noise Pre-Emphasis Vocal Equalization
        └─ Attack/Decay Ballistics & Gravity Peak Physics
                        │
                        ▼
      [ Multi-Color Mixer & Geometry Generator ]
        ├─ Multi-Stop Palette Interpolator (Red, Orange, Violet, Blue)
        ├─ Dynamic Amplitude Glow Boost (RMS)
        ├─ Bar Mesh Generator (Mode 1 & 3)
        └─ 5-Tap Gaussian Filter + Catmull-Rom Bezier Spline (Mode 2)
                        │
                        ▼
   [ Direct2D 1.1 Hardware Accelerated GPU Renderer ]
        ├─ ID2D1LinearGradientBrush (Full-Width Hardware Gradient)
        ├─ Dual-Stroke Rendering (Ambient Glow + Vibrant Core)
        ├─ Translucent Aurora Fill Under Wave
        └─ Target: Layered Transparent Click-Through Window
                        │
                        ▼
         [ Bottom-Screen Monitor Overlay ]
```

---

## Author & Attribution
- **Creator & Lead Architect:** **Amir (Only-One-Kind)**
- **License:** Open for community enhancement and developer modding.
