#pragma once
#include "types.hpp"
#include "color_mixer.hpp"
#include <windows.h>
#include <d2d1.h>
#include <vector>

class VisualizerRenderer {
public:
    VisualizerRenderer();
    ~VisualizerRenderer();

    bool Initialize(HWND hwnd, int width, int height);
    void Resize(int width, int height);
    void Shutdown();

    void Render(const std::vector<float>& bands,
                const std::vector<float>& peak_caps,
                const std::vector<float>& waveform,
                float rms_energy,
                const AppConfig& config,
                const ColorMixer& color_mixer);

private:
    void RenderEqualizerBars(const std::vector<float>& bands,
                            const std::vector<float>& peak_caps,
                            float rms_energy,
                            const AppConfig& config,
                            const ColorMixer& color_mixer);

    void RenderWaveformCurve(const std::vector<float>& bands,
                            const std::vector<float>& waveform,
                            float rms_energy,
                            const AppConfig& config,
                            const ColorMixer& color_mixer);

    void RenderMirroredWave(const std::vector<float>& bands,
                           float rms_energy,
                           const AppConfig& config,
                           const ColorMixer& color_mixer);

    void UpdateGradientBrush(const ColorMixer& color_mixer, float rms_energy, float glow_boost);
    void PresentToWindow();

    HWND m_hwnd;
    int m_width;
    int m_height;

    // Direct2D resources
    ID2D1Factory* m_d2d_factory;
    ID2D1DCRenderTarget* m_dc_target;
    ID2D1SolidColorBrush* m_solid_brush;
    ID2D1LinearGradientBrush* m_gradient_brush;

    // Smooth curve cache for Mode 2
    std::vector<float> m_curve_smoothed;

    // GDI backbuffer for layered window
    HDC m_mem_dc;
    HBITMAP m_bitmap;
    HBITMAP m_old_bitmap;
    void* m_pixel_bits;
};
