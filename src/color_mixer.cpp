#include "color_mixer.hpp"
#include <cstdio>
#include <cstdlib>

ColorMixer::ColorMixer() {
    SetDefaultPalette();
}

D2D1_COLOR_F ColorMixer::HexToColor(const std::string& hex, float default_alpha) {
    if (hex.empty()) return D2D1::ColorF(1.0f, 1.0f, 1.0f, default_alpha);

    const char* str = hex.c_str();
    if (str[0] == '#') str++;

    unsigned int val = 0;
    val = (unsigned int)std::strtoul(str, nullptr, 16);

    size_t len = strlen(str);
    if (len == 6) {
        float r = ((val >> 16) & 0xFF) / 255.0f;
        float g = ((val >> 8) & 0xFF) / 255.0f;
        float b = (val & 0xFF) / 255.0f;
        return D2D1::ColorF(r, g, b, default_alpha);
    } else if (len == 8) {
        float r = ((val >> 24) & 0xFF) / 255.0f;
        float g = ((val >> 16) & 0xFF) / 255.0f;
        float b = ((val >> 8) & 0xFF) / 255.0f;
        float a = (val & 0xFF) / 255.0f;
        return D2D1::ColorF(r, g, b, a);
    }

    return D2D1::ColorF(1.0f, 1.0f, 1.0f, default_alpha);
}

void ColorMixer::SetDefaultPalette() {
    m_stops.clear();
    m_stops.push_back({ 0.00f, HexToColor("#FF1A4B"), "#FF1A4B" }); // Crimson Bass (Red)
    m_stops.push_back({ 0.30f, HexToColor("#FF5722"), "#FF5722" }); // Orange
    m_stops.push_back({ 0.65f, HexToColor("#8B5CF6"), "#8B5CF6" }); // Violet Mids (Purple)
    m_stops.push_back({ 1.00f, HexToColor("#00A3FF"), "#00A3FF" }); // Azure Neon (Blue)
}

void ColorMixer::SetPalette(const std::vector<ColorStop>& stops) {
    if (stops.size() < 2) {
        SetDefaultPalette();
        return;
    }
    m_stops = stops;
    std::sort(m_stops.begin(), m_stops.end(), [](const ColorStop& a, const ColorStop& b) {
        return a.position < b.position;
    });
}

D2D1_COLOR_F ColorMixer::GetColorAt(float t) const {
    if (m_stops.empty()) return D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_stops.size() == 1) return m_stops[0].color;

    t = std::clamp(t, 0.0f, 1.0f);

    if (t <= m_stops.front().position) return m_stops.front().color;
    if (t >= m_stops.back().position) return m_stops.back().color;

    for (size_t i = 0; i < m_stops.size() - 1; ++i) {
        if (t >= m_stops[i].position && t <= m_stops[i + 1].position) {
            float p0 = m_stops[i].position;
            float p1 = m_stops[i + 1].position;
            float span = p1 - p0;
            if (span <= 0.0001f) return m_stops[i].color;

            float rel = (t - p0) / span;
            // Smoothstep curve for seamless blending
            float s = rel * rel * (3.0f - 2.0f * rel);

            const auto& c0 = m_stops[i].color;
            const auto& c1 = m_stops[i + 1].color;

            float r = c0.r + (c1.r - c0.r) * s;
            float g = c0.g + (c1.g - c0.g) * s;
            float b = c0.b + (c1.b - c0.b) * s;
            float a = c0.a + (c1.a - c0.a) * s;

            return D2D1::ColorF(r, g, b, a);
        }
    }

    return m_stops.back().color;
}

D2D1_COLOR_F ColorMixer::GetBandColor(int band_index, int total_bands) const {
    if (total_bands <= 1) return GetColorAt(0.0f);
    float t = static_cast<float>(band_index) / static_cast<float>(total_bands - 1);
    return GetColorAt(t);
}

D2D1_COLOR_F ColorMixer::ModulateGlow(D2D1_COLOR_F base, float rms_energy, float boost_factor) const {
    float glow = 1.0f + std::clamp(rms_energy * boost_factor * 2.0f, 0.0f, 0.40f);
    float r = std::min(1.0f, base.r * glow);
    float g = std::min(1.0f, base.g * glow);
    float b = std::min(1.0f, base.b * glow);
    return D2D1::ColorF(r, g, b, base.a);
}
