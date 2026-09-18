#pragma once
#include "types.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

class ColorMixer {
public:
    ColorMixer();
    void SetPalette(const std::vector<ColorStop>& stops);
    void SetDefaultPalette();

    // Evaluates color at normalized position t (0.0 to 1.0)
    D2D1_COLOR_F GetColorAt(float t) const;

    // Evaluates color for a specific frequency band
    D2D1_COLOR_F GetBandColor(int band_index, int total_bands) const;

    // Modulates color brightness with RMS energy
    D2D1_COLOR_F ModulateGlow(D2D1_COLOR_F base, float rms_energy, float boost_factor) const;

    // Hex helper
    static D2D1_COLOR_F HexToColor(const std::string& hex, float default_alpha = 1.0f);

private:
    std::vector<ColorStop> m_stops;
};
