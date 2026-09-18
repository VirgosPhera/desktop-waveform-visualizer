#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <functional>

class AudioCapture {
public:
    using AudioDataCallback = std::function<void(const float* samples, size_t count)>;

    AudioCapture();
    ~AudioCapture();

    bool Start(AudioDataCallback callback);
    void Stop();

    int GetSampleRate() const { return m_sample_rate; }
    int GetChannels() const { return m_channels; }
    bool IsRunning() const { return m_running; }

private:
    void CaptureThread();

    std::atomic<bool> m_running;
    std::thread m_thread;
    AudioDataCallback m_callback;

    int m_sample_rate;
    int m_channels;
    int m_bits_per_sample;
    WORD m_format_tag;
};
