#if defined(__linux__)
#include "audio_capture.hpp"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <iostream>
#include <vector>

AudioCapture::AudioCapture()
    : m_running(false), m_sample_rate(48000), m_channels(2),
      m_bits_per_sample(32), m_format_tag(3) {}

AudioCapture::~AudioCapture() {
    Stop();
}

bool AudioCapture::Start(AudioDataCallback callback) {
    if (m_running) return true;
    m_callback = callback;
    m_running = true;
    m_thread = std::thread(&AudioCapture::CaptureThread, this);
    return true;
}

void AudioCapture::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void AudioCapture::CaptureThread() {
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = m_sample_rate;
    ss.channels = m_channels;

    int error = 0;
    // Connect to default output monitor sink (works with PulseAudio & PipeWire)
    pa_simple* s = pa_simple_new(
        NULL,                        // Default server
        "AetherWave",                // Application name
        PA_STREAM_RECORD,            // Record stream
        "@DEFAULT_SINK@.monitor",    // Default output loopback monitor
        "Desktop Audio Loopback",    // Stream description
        &ss,                         // Sample spec (48kHz stereo float)
        NULL,                        // Channel map
        NULL,                        // Buffering attributes
        &error
    );

    if (!s) {
        // Fallback to default recording source
        s = pa_simple_new(NULL, "AetherWave", PA_STREAM_RECORD, NULL,
                          "Fallback Capture", &ss, NULL, NULL, &error);
        if (!s) {
            std::cerr << "[AetherWave Linux] Failed to connect to PulseAudio/PipeWire: "
                      << pa_strerror(error) << std::endl;
            return;
        }
    }

    const size_t frames_per_read = 512;
    std::vector<float> interleaved(frames_per_read * m_channels);
    std::vector<float> mono(frames_per_read);

    while (m_running) {
        if (pa_simple_read(s, interleaved.data(), interleaved.size() * sizeof(float), &error) < 0) {
            std::cerr << "[AetherWave Linux] pa_simple_read() failed: " << pa_strerror(error) << std::endl;
            break;
        }

        // Downmix stereo float to mono
        for (size_t f = 0; f < frames_per_read; ++f) {
            mono[f] = 0.5f * (interleaved[f * 2] + interleaved[f * 2 + 1]);
        }

        if (m_callback) {
            m_callback(mono.data(), mono.size());
        }
    }

    pa_simple_free(s);
}
#endif
