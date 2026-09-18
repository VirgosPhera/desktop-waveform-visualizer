---
version: alpha
name: AetherWave
description: Bottom-screen desktop audio visualizer with dynamic multi-color blending and strict height constraints.
author: Amir (Only-One-Kind)
colors:
  primary: "#FF1A4B"
  secondary: "#00A3FF"
  tertiary: "#8B5CF6"
  neutral: "#08080C"
  surface: "#12131A"
  accent-amber: "#FF6B00"
  accent-cyan: "#00F0FF"
typography:
  caption:
    fontFamily: Segoe UI
    fontSize: 11px
    fontWeight: 500
    lineHeight: 1.2
    letterSpacing: "0.02em"
rounded:
  sm: 2px
  md: 4px
  lg: 8px
spacing:
  xs: 2px
  sm: 4px
  md: 8px
  lg: 16px
components:
  visualizer-container:
    backgroundColor: "{colors.neutral}"
    height: 106px
  equalizer-bar:
    backgroundColor: "{colors.primary}"
    rounded: "{rounded.sm}"
  waveform-stroke:
    backgroundColor: "{colors.secondary}"
  peak-cap:
    backgroundColor: "{colors.accent-cyan}"
    rounded: "{rounded.sm}"
  glow-accent:
    backgroundColor: "{colors.accent-amber}"
---

# DESIGN — Visual Design Tokens & Aesthetic Specifications

> **Lead Architect & Original Author:** Amir (Only-One-Kind)  
> **System Name:** AetherWave  
> **Version:** 1.1.0  
> **Repository:** `desktop-waveform-visualizer`

---

## Overview
AetherWave dirancang sebagai elemen visual ambient desktop yang terikat di bagian paling bawah layar monitor. Filosofi desainnya mengedepankan presisi teknis, keterbacaan spektrum audio instan, dan integrasi desktop tanpa hambatan (*zero disruption*). Visualizer ini menggunakan ruang sempit (tinggi dibatasi 1.5–6.0 cm / default 2.8 cm / ~106 px) untuk menyajikan dinamika audio dengan palet multi-warna yang kaya namun terkontrol, memadukan merah tajam, ungu elektrik, dan biru neon tanpa degradasi visual atau kekacauan warna.

---

## Colors & Acoustic Mapping

- **Primary (`#FF1A4B` — Crimson Red):** Merepresentasikan zona energi frekuensi rendah (Sub-Bass dan Bass 20–250 Hz). Warna ini menjadi jangkar visual di sisi kiri visualizer saat terjadi hentakan kick drum atau bass drop.
- **Secondary (`#00A3FF` — Azure Neon Blue):** Merepresentasikan zona frekuensi tinggi (Treble dan Brilliance 4.000–20.000 Hz). Menghiasi sisi kanan visualizer dengan kilauan reaktif saat desis simbal atau snare terdengar.
- **Tertiary (`#8B5CF6` — Electric Violet):** Jembatan harmonis untuk rentang vokal dan instrumen menengah (Mid-Range 250–4.000 Hz). Terbentuk secara alami dari pencampuran spektrum merah dan biru.
- **Neutral (`#08080C`):** Warna latar belakang canvas dengan per-pixel alpha transparency (hampir 100% transparan) agar tidak menghalangi wallpaper desktop atau taskbar.
- **Surface (`#12131A`):** Warna dasar untuk context menu system tray dan dialog konfigurasi.
- **Accent Amber (`#FF6B00`) & Accent Cyan (`#00F0FF`):** Aksen sekunder pada mode palet Cyberpunk dan modulasi kecerahan dinamis saat audio mencapai puncak RMS.

---

## Typography
- **Caption (Segoe UI, 11px, 500 weight):** Digunakan secara eksklusif pada context menu system tray, tooltip tray, dan status indicator profile. Visualizer utama di layar bersifat murni visual tanpa teks agar tidak mengotori desktop.

---

## Layout & Spatial Geometry

- **Docking:** Menempel presisi tepat 2–3 px di atas taskbar Windows (`Shell_TrayWnd`), panel Linux (`_NET_WORKAREA`), atau macOS Dock (`visibleFrame`).
- **Dimensi Tinggi (Constraint):** Rentang 1.5 cm hingga 6.0 cm (default: 2.8 cm / ~106 px pada 96 DPI).
- **Lebar:** 100% bentang monitor aktif (Full Width).
- **Equalizer Spacing:** Jarak antar-bar $S = 2.0\text{ px} - 4.0\text{ px}$ (default 3.0 px) untuk memastikan pemisahan visual yang tajam.

---

## Elevation & Depth

- **Z-Order:** Paling atas secara visual (`WS_EX_TOPMOST`), namun tidak pernah menjadi foreground window aktif (`WS_EX_NOACTIVATE`).
- **Mouse Interaction:** Zero elevation collision (`WS_EX_TRANSPARENT`). Seluruh event mouse klik kiri, klik kanan, double click, dan scroll menembus ke jendela di belakangnya.
- **Dynamic Amplitude Glow:** Modulasi kilau berbasis Direct2D Linear Gradient Brush di sekitar bar atau kurva saat amplitude RMS tinggi, tanpa bayangan pekat (drop-shadow) yang membuat desktop terlihat kotor.

---

## Shapes & Geometry

- **Equalizer Bar:** Rounded rectangle dengan sudut tumpul radius 2.0–2.5 px untuk menghindari kesan tajam yang kaku.
- **Peak Cap:** Garis horizontal tipis setebal 2 px dengan rounded cap.
- **Waveform Spline:** Garis kurva continuous ultra-halus menggunakan interpolasi Catmull-Rom Cubic Bezier berketebalan 2.5 px dengan 5-tap Gaussian spatial smoothing dan smoothstep edge tapering.

---

## Author & Attribution
- **Creator & Lead Architect:** **Amir (Only-One-Kind)**
- **System:** AetherWave Design Tokens v1.1.0
