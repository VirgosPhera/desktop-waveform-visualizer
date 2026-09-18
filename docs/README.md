# AetherWave — Documentation Suite & Developer Index

> **Creator & Lead Architect:** Amir (Only-One-Kind)  
> **System Name:** AetherWave Desktop Audio Visualizer  
> **Version:** 1.1.0  
> **Target OS:** Windows 10/11, Linux (Ubuntu/Debian/Arch/Fedora), macOS (Apple Silicon & Intel)

---

## 📚 Available Documentation

Welcome to the complete engineering documentation for **AetherWave**. All documents are designed to give developers, modders, and users full transparency into how the application is built and how to customize or extend it.

| Document | Description | Direct Link |
| :--- | :--- | :--- |
| **Manual Editing Guide** | **Step-by-step instructions for editing without compiling (`config.json`) and editing C++ source code (DSP, smoothing, custom modes, physics).** | [`MANUAL_EDITING_GUIDE.md`](./MANUAL_EDITING_GUIDE.md) |
| **System Architecture** | **High-level architecture diagrams, DSP math, audio pipeline, Direct2D renderer, and Single-Instance concurrency specs.** | [`ARCHITECTURE.md`](./ARCHITECTURE.md) |
| **Product Requirements (PRD)** | **Formal PRD covering Functional Requirements (FR-001 to FR-012), Non-Functional Requirements, and user flows.** | [`PRD.md`](./PRD.md) |
| **Technical Specs (SPECS)** | **Comprehensive mathematical formulas (FFT, Hann window, Pink Noise vocal curve, Catmull-Rom Bezier tangents, DPI scaling).** | [`SPECS.md`](./SPECS.md) |
| **Feature Roadmap (ROADMAP)** | **Feature hierarchy, status breakdown across modules F-01 to F-06, and version milestone matrices.** | [`ROADMAP.md`](./ROADMAP.md) |
| **Design Specifications (DESIGN)** | **Google DESIGN.md design tokens, acoustic color mapping, typography, elevation, and component specs.** | [`DESIGN.md`](./DESIGN.md) |

---

## 🚀 Quick Start for Developers & Modders

### 1. I just want to change colors, height, or modes (No coding required):
Read the **[Manual Editing Guide — No-Code Customization](./MANUAL_EDITING_GUIDE.md#2-no-code-customization-live-json-config)**.  
Edit `config/config.json` (or `bin/config/config.json`) and save. The app hot-reloads within 50ms!

### 2. I want to tweak the audio dynamics or vocal bounce in C++:
Read **[How to Tweak Audio DSP & Vocal Curves](./MANUAL_EDITING_GUIDE.md#how-to-tweak-audio-dsp--vocal-curves-srcdspcpp)**.  
Modify `src/dsp.cpp` and re-run `build.bat` (Windows) or `cmake --build build` (Linux/macOS).

### 3. I want to tweak the Waveform Curve smoothness or physics:
Read **[How to Customize Waveform Curve Smoothing](./MANUAL_EDITING_GUIDE.md#how-to-customize-waveform-curve-smoothing-srcrenderercpp)**.  
Modify the Gaussian spatial kernel, temporal ballistics, or spline weights in `src/renderer.cpp`.

### 4. I want to create a brand new 4th visualizer mode:
Read **[How to Add a Brand New Visualizer Mode](./MANUAL_EDITING_GUIDE.md#how-to-add-a-brand-new-visualizer-mode-step-by-step)**.

---

## 👑 Author & Credits
- **Lead Architect & Original Author:** **Amir (Only-One-Kind)**
- **Role:** Complete software design, mathematical formulation, DSP vocal pre-emphasis curve design, Direct2D/Cairo/Cocoa multi-platform architecture, single-instance mutex implementation, and documentation.
- **License:** Open for community modification. Credit to Amir (Only-One-Kind) must be retained in all forks and distributions.
