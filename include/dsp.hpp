#pragma once
#include "types.hpp"
#include <vector>
#include <complex>

class DspEngine {
public:
    DspEngine(int fft_size = 1024, int sample_rate = 48000, int num_bands = 64);

    void Reset(int fft_size, int sample_rate, int num_bands);

    // Feed audio samples into DSP (mono float)
    void ProcessSamples(const float* samples, size_t count);

    // Update ballistics and physics for frame (dt in seconds)
    void Update(float dt, const AppConfig& config);

    // Spectrum outputs
    const std::vector<float>& GetBands() const { return m_smooth_bands; }
    const std::vector<float>& GetPeakCaps() const { return m_peak_caps; }
    const std::vector<float>& GetWaveformPoints() const { return m_waveform_display; }

    float GetRms() const { return m_smooth_rms; }

private:
    void ComputeFFT();
    void ComputeLogBands(const AppConfig& config);

    int m_fft_size;
    int m_sample_rate;
    int m_num_bands;

    std::vector<float> m_hann_window;
    std::vector<float> m_input_ring;
    size_t m_ring_write_pos;

    // Complex FFT buffers
    std::vector<std::complex<float>> m_fft_buffer;
    std::vector<size_t> m_bit_reverse;

    // Magnitudes
    std::vector<float> m_raw_magnitudes;
    std::vector<float> m_raw_bands;
    std::vector<float> m_smooth_bands;

    // Peak Caps
    std::vector<float> m_peak_caps;
    std::vector<float> m_peak_velocity;
    std::vector<float> m_peak_hold_timer;

    // Waveform curve display points
    std::vector<float> m_waveform_display;

    // Band bin boundaries
    struct BandRange {
        int bin_start;
        int bin_end;
        float eq_boost;
    };
    std::vector<BandRange> m_band_ranges;

    float m_current_rms;
    float m_smooth_rms;
};
