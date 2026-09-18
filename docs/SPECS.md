# SPECS — Full Technical Specifications & Mathematical Foundations

> **Lead Architect & Original Author:** Amir (Only-One-Kind)  
> **System Name:** AetherWave  
> **Version:** 1.1.0  
> **Repository:** `desktop-waveform-visualizer`

---

## 1. Feature Spec: Audio Capture (F-01)

### 1. Objective
Provide passive, zero-cable, ultra-low latency (<10ms) system audio loopback capture across Windows, Linux, and macOS without requiring virtual audio routing cables or interfering with existing user audio streams.

### 2. Implementation Details
- **Windows:** WASAPI Loopback Capture (`AUDCLNT_STREAMFLAGS_LOOPBACK`) querying `GetDefaultAudioEndpoint(eRender, eConsole)`. Automatically converts IEEE 32-bit float or 16/24-bit PCM stereo (44.1kHz/48kHz) to a normalized internal mono float buffer `[-1.0f, 1.0f]`.
- **Linux:** PulseAudio Simple API (`libpulse-simple`) capturing directly from `@DEFAULT_SINK@.monitor`.
- **macOS:** CoreAudio AudioQueue stream.
- High-priority realtime capture thread writing into a lock-free Single Producer Single Consumer (SPSC) Circular Ring Buffer (8192 samples).

### 3. Rules & Constraints
- ZERO locks or mutex acquisitions in the capture write path.
- Fallback silence padding (0.0f) if playback pauses so the visualizer returns smoothly to baseline.

### 4. Acceptance Criteria
- Seamless automatic capture from YouTube Music, Pear-Desktop, Spotify, browsers, and games.
- End-to-end latency from audio playback to ring buffer < 10ms.
- Capture thread CPU consumption < 0.2% on a single core.

---

## 2. Feature Spec: DSP & Equal-Loudness Vocal Engine (F-02)

### 1. Objective
Transform time-domain PCM samples into high-resolution frequency domain data using Fast Fourier Transform, compensate for acoustic pink-noise slope so vocals and melodies dance vigorously, and apply smooth physics ballistics.

### 2. Implementation Details
- **Window Size:** 1024 points.
- **Window Function:** Hann Window:
  $$w(n) = 0.5 \times \left(1 - \cos\left(\frac{2\pi n}{N - 1}\right)\right)$$
- **Cooley-Tukey Radix-2 FFT:** Real-to-complex transform computing $N/2$ magnitude bins:
  $$\text{Magnitude}_k = \frac{\sqrt{\text{Re}_k^2 + \text{Im}_k^2}}{N}$$
- **Logarithmic Frequency Grouping:** Maps 20 Hz – 20,000 Hz into 64 logarithmic bands:
  $$f(i) = f_{\min} \times \left(\frac{f_{\max}}{f_{\min}}\right)^{i / 64}$$
- **Acoustic Pre-Emphasis (Vocal Balancing Curve):**
  Real musical recordings naturally decrease in energy at ~3 to 4.5 dB per octave (Pink Noise slope). To ensure singer vocals and lead instruments move dynamically alongside bass kicks, piecewise pre-emphasis $W(f)$ is applied:
  - Sub-bass ($< 150\text{ Hz}$): $1.0\times - 1.4\times$
  - Mid-bass ($150 - 350\text{ Hz}$): $1.4\times - 3.4\times$
  - Low vocals ($350 - 800\text{ Hz}$): $+3.4\times - 7.0\times$
  - Core vocals ($800 - 2500\text{ Hz}$): $+7.0\times - 12.0\times$
  - Vocal harmonics & guitars ($2.5 - 6\text{ kHz}$): $+12.0\times - 16.5\times$
  - Treble & air ($6 - 20\text{ kHz}$): $+16.5\times - 22.0\times$
- **Decibel Compression:**
  $$\text{dB} = 20 \log_{10}(\text{Magnitude} \times W(f) + 10^{-6})$$
  Scaled organically between $-44.0\text{ dB}$ (noise floor) and $-2.0\text{ dB}$ (ceiling).
- **Ballistics Dampening (EMA):**
  - Attack: $\tau_{\text{attack}} = 12\text{ms}$ (rapid onset response)
  - Decay: $\tau_{\text{decay}} = 140\text{ms}$ (gentle organic release)
- **Floating Peak Caps:**
  Peak velocity accelerates with simulated gravity ($g = 9.8 \times \text{scale}$) following a 200ms peak hold timer.

---

## 3. Feature Spec: Display, Docking & Click-Through (F-03)

### 1. Objective
Render a lightweight transparent overlay strictly at the bottom of the screen above the taskbar or desktop panel, with physical height controlled between 1.5 cm and 6.0 cm, completely impervious to mouse interference.

### 2. Implementation Details
- **Win32 Window Styles:**
  `WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW`.
- **Dynamic Taskbar Detection:**
  Queries `FindWindowW(L"Shell_TrayWnd", NULL)` every 0.5 seconds:
  $$\text{Top} = \text{rcTaskbar.top} - \text{VisualizerHeight} - \text{margin\_px}$$
  Provides a guaranteed 2–3 px safety gap so the visualizer NEVER collides with or covers taskbar icons.
- **Linux:** Automatically reads `_NET_WORKAREA` atom from root window to dock above panels with a 2px gap.
- **macOS:** Automatically queries `[NSScreen mainScreen].visibleFrame` to dock neatly above the macOS Dock.
- **Single-Instance Mutex:**
  Named Mutex `Local\AetherWave_SingleInstance_Mutex` (Windows) and `flock` lockfile (POSIX) preventing duplicate processes from running.
- **Per-Monitor V2 DPI Scaling:**
  $$\text{HeightPixels} = \frac{\text{HeightCM}}{2.54} \times \text{DPI}$$
  Default: 2.8 cm (~106 px at 96 DPI).

---

## 4. Feature Spec: Visualizer Modes & Smoothing (F-04)

### 1. Mode 1: Equalizer Bars (`equalizer_bars`)
- 64 rounded rectangles (`D2D1_ROUNDED_RECT`) with 2.5px radius.
- Floating peak caps hovering above each bar.
- Individual color mapped per frequency band from Bass Red to Treble Blue.

### 2. Mode 2: Waveform Curve (`waveform_curve`)
- **5-Tap Gaussian Spatial Filter:**
  $$S_i = 0.06 B_{i-2} + 0.24 B_{i-1} + 0.40 B_i + 0.24 B_{i+1} + 0.06 B_{i+2}$$
  Completely eliminates sharp triangular peaks and saw-tooth jaggedness.
- **Smoothstep Edge Tapering:**
  Tapers the first 5 and last 5 points to 0 at monitor borders using smoothstep $3t^2 - 2t^3$.
- **Liquid Inertia Ballistics:**
  Attack factor 0.32, decay factor 0.14 for luscious, fluid-like momentum.
- **Monotonic Catmull-Rom Bezier Splines:**
  Tangents clamped between $P_1$ and $P_2$ to guarantee monotonicity in X and prevent backward loops.
- **Dual Stroke + Aurora Fill:**
  - Translucent gradient fill underneath down to screen floor.
  - Soft ambient outer glow stroke (width: `wave_thickness + 3px`, alpha: 35%).
  - Sharp vibrant core stroke (width: `wave_thickness`, alpha: 95%).

### 3. Mode 3: Mirrored Wave (`mirrored_wave`)
- Centered baseline ($Y = \text{Height} / 2$).
- Symmetrical vertical spectrum bars radiating upward and downward.

---

## 5. Feature Spec: Multi-Color Mixer (F-05)

### 1. Multi-Stop Palette Interpolation
Normalized position $t \in [0.0, 1.0]$:
- $t = 0.00$: Crimson Bass (`#FF1A4B`)
- $t = 0.30$: Orange Mid-Bass (`#FF5722`)
- $t = 0.65$: Electric Violet Vocals (`#8B5CF6`)
- $t = 1.00$: Azure Neon Treble (`#00A3FF`)

Between stops, color components $(R, G, B)$ interpolate using the smoothstep Hermite polynomial:
$$S(u) = 3u^2 - 2u^3, \quad u = \frac{t - t_a}{t_b - t_a}$$

### 2. Dynamic Amplitude Glow Boost
Modulates luminance and saturation based on realtime RMS volume:
$$\text{Color}_{\text{glow}} = \text{Color}_{\text{base}} \times (1.0 + \text{RMS} \times \text{boost\_factor})$$
Creates a pulsing, radiant glow during energetic drum fills and vocal climaxes.

---

## Author & Attribution
- **Creator & Lead Architect:** **Amir (Only-One-Kind)**
- **System:** AetherWave Engine v1.1.0
