#include "renderer.hpp"
#include <algorithm>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

VisualizerRenderer::VisualizerRenderer()
    : m_hwnd(NULL), m_width(0), m_height(0),
      m_d2d_factory(nullptr), m_dc_target(nullptr),
      m_solid_brush(nullptr), m_gradient_brush(nullptr),
      m_mem_dc(NULL), m_bitmap(NULL), m_old_bitmap(NULL),
      m_pixel_bits(nullptr) {}

VisualizerRenderer::~VisualizerRenderer() {
    Shutdown();
}

bool VisualizerRenderer::Initialize(HWND hwnd, int width, int height) {
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d_factory);
    if (FAILED(hr) || !m_d2d_factory) return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    );

    hr = m_d2d_factory->CreateDCRenderTarget(&props, &m_dc_target);
    if (FAILED(hr) || !m_dc_target) return false;

    m_dc_target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    hr = m_dc_target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &m_solid_brush);
    if (FAILED(hr)) return false;

    Resize(width, height);
    return true;
}

void VisualizerRenderer::Resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    m_width = width;
    m_height = height;

    HDC screen_dc = GetDC(NULL);
    if (!m_mem_dc) {
        m_mem_dc = CreateCompatibleDC(screen_dc);
    }

    if (m_bitmap) {
        SelectObject(m_mem_dc, m_old_bitmap);
        DeleteObject(m_bitmap);
        m_bitmap = NULL;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_width;
    bmi.bmiHeader.biHeight = -m_height; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_bitmap = CreateDIBSection(m_mem_dc, &bmi, DIB_RGB_COLORS, &m_pixel_bits, NULL, 0);
    m_old_bitmap = (HBITMAP)SelectObject(m_mem_dc, m_bitmap);

    ReleaseDC(NULL, screen_dc);
}

void VisualizerRenderer::Shutdown() {
    if (m_gradient_brush) {
        m_gradient_brush->Release();
        m_gradient_brush = nullptr;
    }
    if (m_solid_brush) {
        m_solid_brush->Release();
        m_solid_brush = nullptr;
    }
    if (m_dc_target) {
        m_dc_target->Release();
        m_dc_target = nullptr;
    }
    if (m_d2d_factory) {
        m_d2d_factory->Release();
        m_d2d_factory = nullptr;
    }
    if (m_mem_dc && m_bitmap) {
        SelectObject(m_mem_dc, m_old_bitmap);
        DeleteObject(m_bitmap);
        DeleteDC(m_mem_dc);
        m_mem_dc = NULL;
        m_bitmap = NULL;
    }
}

void VisualizerRenderer::Render(const std::vector<float>& bands,
                                const std::vector<float>& peak_caps,
                                const std::vector<float>& waveform,
                                float rms_energy,
                                const AppConfig& config,
                                const ColorMixer& color_mixer) {
    if (!m_dc_target || !m_mem_dc || m_width <= 0 || m_height <= 0) return;

    RECT rc = { 0, 0, m_width, m_height };
    HRESULT hr = m_dc_target->BindDC(m_mem_dc, &rc);
    if (FAILED(hr)) return;

    m_dc_target->BeginDraw();
    m_dc_target->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    switch (config.mode) {
        case VisualizerMode::EQUALIZER_BARS:
            RenderEqualizerBars(bands, peak_caps, rms_energy, config, color_mixer);
            break;
        case VisualizerMode::WAVEFORM_CURVE:
            RenderWaveformCurve(bands, waveform, rms_energy, config, color_mixer);
            break;
        case VisualizerMode::MIRRORED_WAVE:
            RenderMirroredWave(bands, rms_energy, config, color_mixer);
            break;
    }

    hr = m_dc_target->EndDraw();
    if (SUCCEEDED(hr)) {
        PresentToWindow();
    }
}

void VisualizerRenderer::RenderEqualizerBars(const std::vector<float>& bands,
                                             const std::vector<float>& peak_caps,
                                             float rms_energy,
                                             const AppConfig& config,
                                             const ColorMixer& color_mixer) {
    size_t num_bars = bands.size();
    if (num_bars == 0) return;

    float spacing = config.bar_spacing_px;
    float total_spacing = spacing * (num_bars - 1);
    float avail_width = static_cast<float>(m_width) - total_spacing;
    float bar_width = std::max(2.0f, avail_width / num_bars);

    float max_bar_h = static_cast<float>(m_height) - 12.0f;
    float radius = config.corner_radius_px;

    for (size_t i = 0; i < num_bars; ++i) {
        float h = bands[i] * max_bar_h;
        if (h < 2.0f) h = 2.0f; // Minimal aesthetic bar height

        float left = i * (bar_width + spacing);
        float top = static_cast<float>(m_height) - h;
        float right = left + bar_width;
        float bottom = static_cast<float>(m_height);

        D2D1_COLOR_F base_color = color_mixer.GetBandColor(static_cast<int>(i), static_cast<int>(num_bars));
        D2D1_COLOR_F bar_color = color_mixer.ModulateGlow(base_color, rms_energy, config.glow_boost);
        bar_color.a = config.base_alpha;

        m_solid_brush->SetColor(bar_color);

        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(
            D2D1::RectF(left, top, right, bottom),
            radius, radius
        );
        m_dc_target->FillRoundedRectangle(rounded_rect, m_solid_brush);

        // Peak Cap indicator
        if (config.peak_cap_enabled && i < peak_caps.size()) {
            float peak_h = peak_caps[i] * max_bar_h;
            float peak_top = static_cast<float>(m_height) - peak_h - 4.0f;
            float peak_bottom = peak_top + 2.0f;

            if (peak_top >= 2.0f && peak_bottom < static_cast<float>(m_height)) {
                D2D1_COLOR_F peak_color = bar_color;
                peak_color.r = std::min(1.0f, peak_color.r * 1.3f);
                peak_color.g = std::min(1.0f, peak_color.g * 1.3f);
                peak_color.b = std::min(1.0f, peak_color.b * 1.3f);
                peak_color.a = 1.0f;

                m_solid_brush->SetColor(peak_color);
                D2D1_RECT_F peak_rect = D2D1::RectF(left, peak_top, right, peak_bottom);
                m_dc_target->FillRectangle(peak_rect, m_solid_brush);
            }
        }
    }
}

void VisualizerRenderer::UpdateGradientBrush(const ColorMixer& color_mixer, float rms_energy, float glow_boost) {
    if (!m_dc_target || m_width <= 0) return;

    if (m_gradient_brush) {
        m_gradient_brush->Release();
        m_gradient_brush = nullptr;
    }

    D2D1_GRADIENT_STOP stops[4];
    float pos[4] = { 0.00f, 0.30f, 0.65f, 1.00f };
    for (int i = 0; i < 4; ++i) {
        D2D1_COLOR_F c = color_mixer.GetColorAt(pos[i]);
        c = color_mixer.ModulateGlow(c, rms_energy, glow_boost);
        stops[i].position = pos[i];
        stops[i].color = c;
    }

    ID2D1GradientStopCollection* stop_coll = nullptr;
    HRESULT hr = m_dc_target->CreateGradientStopCollection(
        stops, 4, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stop_coll
    );
    if (SUCCEEDED(hr) && stop_coll) {
        m_dc_target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0.0f, 0.0f),
                D2D1::Point2F(static_cast<float>(m_width), 0.0f)
            ),
            stop_coll,
            &m_gradient_brush
        );
        stop_coll->Release();
    }
}

void VisualizerRenderer::RenderWaveformCurve(const std::vector<float>& bands,
                                             const std::vector<float>& waveform,
                                             float rms_energy,
                                             const AppConfig& config,
                                             const ColorMixer& color_mixer) {
    size_t num_bands = bands.size();
    if (num_bands < 4 || m_width <= 0 || m_height <= 0) return;

    // 1. Light 3-tap Spatial Filter: preserves crisp peaks without jagged kinks, zero temporal lag!
    // Directly follows the exact rise and fall speed of Equalizer Bars!
    std::vector<float> curve_heights(num_bands, 0.0f);
    for (size_t i = 0; i < num_bands; ++i) {
        float prev = (i > 0) ? bands[i - 1] : bands[i];
        float curr = bands[i];
        float next = (i + 1 < num_bands) ? bands[i + 1] : bands[i];
        curve_heights[i] = prev * 0.15f + curr * 0.70f + next * 0.15f;
    }

    // 2. Smoothstep edge tapering for the 3 outer points so ends cleanly touch baseline
    const size_t taper_count = 3;
    for (size_t i = 0; i < num_bands; ++i) {
        if (i < taper_count) {
            float t = static_cast<float>(i) / static_cast<float>(taper_count);
            curve_heights[i] *= (t * t * (3.0f - 2.0f * t));
        } else if (i >= num_bands - taper_count) {
            float t = static_cast<float>(num_bands - 1 - i) / static_cast<float>(taper_count);
            curve_heights[i] *= (t * t * (3.0f - 2.0f * t));
        }
    }

    // 3. Update linear gradient brush with glowing multi-colors
    UpdateGradientBrush(color_mixer, rms_energy, config.glow_boost);
    if (!m_gradient_brush) return;

    // 4. Generate control points along the bottom baseline — strictly synced with Equalizer Bars speed!
    float baseline_y = static_cast<float>(m_height) - 3.0f;
    float max_amplitude = static_cast<float>(m_height) * 0.88f;
    float step_x = static_cast<float>(m_width) / static_cast<float>(num_bands - 1);

    std::vector<D2D1_POINT_2F> points(num_bands);
    for (size_t i = 0; i < num_bands; ++i) {
        float x = i * step_x;
        float y = baseline_y - curve_heights[i] * max_amplitude;
        y = std::clamp(y, 4.0f, baseline_y);
        points[i] = D2D1::Point2F(x, y);
    }

    // 7. Construct Bezier Geometry
    ID2D1PathGeometry* path = nullptr;
    HRESULT hr = m_d2d_factory->CreatePathGeometry(&path);
    if (FAILED(hr) || !path) return;

    ID2D1GeometrySink* sink = nullptr;
    hr = path->Open(&sink);
    if (FAILED(hr) || !sink) {
        path->Release();
        return;
    }

    sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);

    for (size_t i = 0; i < num_bands - 1; ++i) {
        D2D1_POINT_2F p0 = (i == 0) ? points[0] : points[i - 1];
        D2D1_POINT_2F p1 = points[i];
        D2D1_POINT_2F p2 = points[i + 1];
        D2D1_POINT_2F p3 = (i + 2 < num_bands) ? points[i + 2] : p2;

        float cp1x = p1.x + (p2.x - p0.x) / 6.0f;
        float cp1y = p1.y + (p2.y - p0.y) / 6.0f;
        float cp2x = p2.x - (p3.x - p1.x) / 6.0f;
        float cp2y = p2.y - (p3.y - p1.y) / 6.0f;

        // Clamp control points to preserve monotonicity in X and avoid overshoot
        cp1x = std::clamp(cp1x, p1.x, p2.x);
        cp2x = std::clamp(cp2x, p1.x, p2.x);
        cp1y = std::clamp(cp1y, 2.0f, baseline_y);
        cp2y = std::clamp(cp2y, 2.0f, baseline_y);

        sink->AddBezier(D2D1::BezierSegment(
            D2D1::Point2F(cp1x, cp1y),
            D2D1::Point2F(cp2x, cp2y),
            p2
        ));
    }

    if (config.fill_under_wave) {
        sink->AddLine(D2D1::Point2F(static_cast<float>(m_width), static_cast<float>(m_height)));
        sink->AddLine(D2D1::Point2F(0.0f, static_cast<float>(m_height)));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    } else {
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }

    sink->Close();
    sink->Release();

    // 8. Draw semi-transparent gradient fill
    if (config.fill_under_wave) {
        m_gradient_brush->SetOpacity(config.fill_opacity);
        m_dc_target->FillGeometry(path, m_gradient_brush);
    }

    // 9. Soft ambient glow stroke
    m_gradient_brush->SetOpacity(config.base_alpha * 0.35f);
    m_dc_target->DrawGeometry(path, m_gradient_brush, config.wave_thickness_px + 3.0f);

    // 10. Crisp vibrant core curve
    m_gradient_brush->SetOpacity(config.base_alpha);
    m_dc_target->DrawGeometry(path, m_gradient_brush, config.wave_thickness_px);

    path->Release();
}

void VisualizerRenderer::RenderMirroredWave(const std::vector<float>& bands,
                                            float rms_energy,
                                            const AppConfig& config,
                                            const ColorMixer& color_mixer) {
    size_t num_bars = bands.size();
    if (num_bars == 0) return;

    float spacing = config.bar_spacing_px;
    float total_spacing = spacing * (num_bars - 1);
    float avail_width = static_cast<float>(m_width) - total_spacing;
    float bar_width = std::max(2.0f, avail_width / num_bars);

    float center_y = static_cast<float>(m_height) * 0.5f;
    float max_half_h = center_y - 6.0f;
    float radius = config.corner_radius_px;

    for (size_t i = 0; i < num_bars; ++i) {
        float half_h = bands[i] * max_half_h;
        if (half_h < 2.0f) half_h = 2.0f;

        float left = i * (bar_width + spacing);
        float top = center_y - half_h;
        float right = left + bar_width;
        float bottom = center_y + half_h;

        D2D1_COLOR_F base_color = color_mixer.GetBandColor(static_cast<int>(i), static_cast<int>(num_bars));
        D2D1_COLOR_F bar_color = color_mixer.ModulateGlow(base_color, rms_energy, config.glow_boost);
        bar_color.a = config.base_alpha;

        m_solid_brush->SetColor(bar_color);

        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(
            D2D1::RectF(left, top, right, bottom),
            radius, radius
        );
        m_dc_target->FillRoundedRectangle(rounded_rect, m_solid_brush);
    }
}

void VisualizerRenderer::PresentToWindow() {
    if (!m_hwnd || !m_mem_dc) return;

    HDC screen_dc = GetDC(NULL);
    POINT ptDst = { 0, 0 };
    RECT rcWnd;
    GetWindowRect(m_hwnd, &rcWnd);
    ptDst.x = rcWnd.left;
    ptDst.y = rcWnd.top;

    SIZE sizeWnd = { m_width, m_height };
    POINT ptSrc = { 0, 0 };

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA; // Pre-multiplied alpha

    UpdateLayeredWindow(m_hwnd, screen_dc, &ptDst, &sizeWnd, m_mem_dc, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, screen_dc);
}
