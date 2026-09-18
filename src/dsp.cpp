#include "dsp.hpp"
#include <cmath>
#include <algorithm>
#include <numbers>

static const float PI = 3.14159265358979323846f;

DspEngine::DspEngine(int fft_size, int sample_rate, int num_bands)
    : m_ring_write_pos(0), m_current_rms(0.0f), m_smooth_rms(0.0f) {
    Reset(fft_size, sample_rate, num_bands);
}

void DspEngine::Reset(int fft_size, int sample_rate, int num_bands) {
    m_fft_size = fft_size;
    m_sample_rate = sample_rate;
    m_num_bands = num_bands;

    m_hann_window.resize(m_fft_size);
    for (int i = 0; i < m_fft_size; ++i) {
        m_hann_window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (m_fft_size - 1)));
    }

    m_input_ring.assign(m_fft_size * 2, 0.0f);
    m_ring_write_pos = 0;

    m_fft_buffer.resize(m_fft_size);
    m_raw_magnitudes.resize(m_fft_size / 2, 0.0f);

    // Bit reversal precomputation
    m_bit_reverse.resize(m_fft_size);
    int bits = static_cast<int>(std::log2(m_fft_size));
    for (int i = 0; i < m_fft_size; ++i) {
        int rev = 0;
        for (int b = 0; b < bits; ++b) {
            if (i & (1 << b)) rev |= (1 << (bits - 1 - b));
        }
        m_bit_reverse[i] = rev;
    }

    m_raw_bands.assign(m_num_bands, 0.0f);
    m_smooth_bands.assign(m_num_bands, 0.0f);
    m_peak_caps.assign(m_num_bands, 0.0f);
    m_peak_velocity.assign(m_num_bands, 0.0f);
    m_peak_hold_timer.assign(m_num_bands, 0.0f);

    m_waveform_display.assign(128, 0.0f);

    // Precalculate logarithmic band ranges
    m_band_ranges.resize(m_num_bands);
    float min_f = 20.0f;
    float max_f = 20000.0f;
    float bin_width = static_cast<float>(m_sample_rate) / m_fft_size;

    for (int i = 0; i < m_num_bands; ++i) {
        float f_low = min_f * std::pow(max_f / min_f, static_cast<float>(i) / m_num_bands);
        float f_high = min_f * std::pow(max_f / min_f, static_cast<float>(i + 1) / m_num_bands);

        int b_start = std::max(1, static_cast<int>(f_low / bin_width));
        int b_end = std::max(b_start, static_cast<int>(f_high / bin_width));
        b_end = std::min(b_end, (m_fft_size / 2) - 1);

        m_band_ranges[i].bin_start = b_start;
        m_band_ranges[i].bin_end = b_end;

        // Equal-Loudness / Pink Noise Pre-Emphasis Tilt Curve:
        // Real music energy falls off by ~3-4.5 dB per octave.
        // We boost mid/vocal frequencies (300Hz - 4kHz) by 4x - 10x,
        // and treble by 12x - 22x so that vocals, piano, snare, and
        // cymbals bounce with equal visual punch as bass kicks!
        float center_f = 0.5f * (f_low + f_high);
        if (center_f < 80.0f) {
            m_band_ranges[i].eq_boost = 1.0f; // Sub-bass
        } else if (center_f < 250.0f) {
            m_band_ranges[i].eq_boost = 1.6f; // Bass
        } else if (center_f < 800.0f) {
            m_band_ranges[i].eq_boost = 4.5f; // Low Mids & Male Vocal core
        } else if (center_f < 2500.0f) {
            m_band_ranges[i].eq_boost = 8.0f; // Mids & Female Vocal presence
        } else if (center_f < 6000.0f) {
            m_band_ranges[i].eq_boost = 12.0f; // Vocal harmonics, snare, guitar
        } else if (center_f < 12000.0f) {
            m_band_ranges[i].eq_boost = 16.5f; // Treble, hi-hats
        } else {
            m_band_ranges[i].eq_boost = 22.0f; // Air & brilliance
        }
    }
}

void DspEngine::ProcessSamples(const float* samples, size_t count) {
    if (!samples || count == 0) return;

    float sum_sq = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float s = samples[i];
        sum_sq += s * s;
        m_input_ring[m_ring_write_pos] = s;
        m_ring_write_pos = (m_ring_write_pos + 1) % m_input_ring.size();
    }

    m_current_rms = std::sqrt(sum_sq / count);
}

void DspEngine::ComputeFFT() {
    // Copy last fft_size samples from ring buffer with Hann window
    size_t cap = m_input_ring.size();
    size_t start_idx = (m_ring_write_pos + cap - m_fft_size) % cap;

    for (int i = 0; i < m_fft_size; ++i) {
        float s = m_input_ring[(start_idx + i) % cap];
        m_fft_buffer[m_bit_reverse[i]] = std::complex<float>(s * m_hann_window[i], 0.0f);
    }

    // Cooley-Tukey Radix-2 In-Place FFT
    for (int len = 2; len <= m_fft_size; len <<= 1) {
        float angle = -2.0f * PI / len;
        std::complex<float> wlen(std::cos(angle), std::sin(angle));
        int half = len >> 1;

        for (int i = 0; i < m_fft_size; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < half; ++j) {
                std::complex<float> u = m_fft_buffer[i + j];
                std::complex<float> v = m_fft_buffer[i + j + half] * w;
                m_fft_buffer[i + j] = u + v;
                m_fft_buffer[i + j + half] = u - v;
                w *= wlen;
            }
        }
    }

    // Extract Magnitudes
    float norm = 2.0f / m_fft_size;
    for (int i = 0; i < m_fft_size / 2; ++i) {
        float mag = std::abs(m_fft_buffer[i]) * norm;
        m_raw_magnitudes[i] = mag;
    }
}

void DspEngine::ComputeLogBands(const AppConfig& config) {
    for (int i = 0; i < m_num_bands; ++i) {
        const auto& range = m_band_ranges[i];
        
        // Combine max peak and average RMS across bins in this band
        float max_mag = 0.0f;
        float energy_sum = 0.0f;
        int count = 0;
        for (int b = range.bin_start; b <= range.bin_end; ++b) {
            float m = m_raw_magnitudes[b];
            max_mag = std::max(max_mag, m);
            energy_sum += m * m;
            count++;
        }
        float band_rms = (count > 0) ? std::sqrt(energy_sum / count) : 0.0f;
        
        // 70% peak + 30% RMS catches both transients and vocal tone bodies
        float combined = 0.70f * max_mag + 0.30f * band_rms;

        // Apply sensitivity and frequency tilt
        float weighted = combined * range.eq_boost * config.sensitivity;

        // Decibel Scale Dynamic Compression:
        // Converts acoustic power to visual range [-45 dB, 0 dB] -> [0.0, 1.0]
        float db = 20.0f * std::log10(std::max(1e-5f, weighted));
        const float min_db = -44.0f;
        const float max_db = -2.0f;
        float normalized = std::clamp((db - min_db) / (max_db - min_db), 0.0f, 1.0f);

        // Smooth visual gamma curve
        float visual_val = std::pow(normalized, 1.20f);
        m_raw_bands[i] = visual_val;
    }
}

void DspEngine::Update(float dt, const AppConfig& config) {
    ComputeFFT();
    ComputeLogBands(config);

    // RMS smoothing
    m_smooth_rms += (m_current_rms - m_smooth_rms) * std::clamp(dt * 15.0f, 0.0f, 1.0f);

    // Ballistics Exponential Moving Average
    float attack_alpha = 1.0f - std::exp(-dt / (config.attack_time_ms * 0.001f));
    float decay_alpha = 1.0f - std::exp(-dt / (config.decay_time_ms * 0.001f));

    for (int i = 0; i < m_num_bands; ++i) {
        float target = m_raw_bands[i];
        if (target > m_smooth_bands[i]) {
            m_smooth_bands[i] += attack_alpha * (target - m_smooth_bands[i]);
        } else {
            m_smooth_bands[i] -= decay_alpha * (m_smooth_bands[i] - target);
        }
        m_smooth_bands[i] = std::clamp(m_smooth_bands[i], 0.0f, 1.0f);

        // Peak Cap Physics
        if (m_smooth_bands[i] >= m_peak_caps[i]) {
            m_peak_caps[i] = m_smooth_bands[i];
            m_peak_hold_timer[i] = config.peak_hold_ms * 0.001f;
            m_peak_velocity[i] = 0.0f;
        } else {
            if (m_peak_hold_timer[i] > 0.0f) {
                m_peak_hold_timer[i] -= dt;
            } else {
                m_peak_velocity[i] += (config.peak_gravity * 0.001f) * dt;
                m_peak_caps[i] -= m_peak_velocity[i] * dt;
                if (m_peak_caps[i] < m_smooth_bands[i]) {
                    m_peak_caps[i] = m_smooth_bands[i];
                    m_peak_velocity[i] = 0.0f;
                }
            }
        }
        m_peak_caps[i] = std::clamp(m_peak_caps[i], 0.0f, 1.0f);
    }

    // Generate smoothed waveform points for Mode 2
    size_t wave_len = m_waveform_display.size();
    size_t cap = m_input_ring.size();
    size_t start_idx = (m_ring_write_pos + cap - wave_len) % cap;
    for (size_t i = 0; i < wave_len; ++i) {
        float raw_sample = m_input_ring[(start_idx + i) % cap];
        // Smooth transition towards current sample
        m_waveform_display[i] += (raw_sample * config.sensitivity - m_waveform_display[i]) * std::clamp(dt * 30.0f, 0.0f, 1.0f);
        m_waveform_display[i] = std::clamp(m_waveform_display[i], -1.0f, 1.0f);
    }
}
