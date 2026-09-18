#include "audio_capture.hpp"
#include <iostream>
#include <vector>

#pragma comment(lib, "ole32.lib")

AudioCapture::AudioCapture()
    : m_running(false), m_sample_rate(48000), m_channels(2),
      m_bits_per_sample(32), m_format_tag(WAVE_FORMAT_IEEE_FLOAT) {}

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
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return;

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        CoUninitialize();
        return;
    }

    IMMDevice* pDevice = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr) || !pDevice) {
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioClient* pAudioClient = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr) || !pAudioClient) {
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    WAVEFORMATEX* pwfx = nullptr;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr) || !pwfx) {
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    m_sample_rate = pwfx->nSamplesPerSec;
    m_channels = pwfx->nChannels;
    m_bits_per_sample = pwfx->wBitsPerSample;
    m_format_tag = pwfx->wFormatTag;

    bool is_float = false;
    if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        is_float = true;
    } else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE* pEx = (WAVEFORMATEXTENSIBLE*)pwfx;
        if (pEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            is_float = true;
        }
    }

    // Initialize in loopback mode (buffer duration 100ms)
    REFERENCE_TIME hnsBufferDuration = 1000000;
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_LOOPBACK,
                                  hnsBufferDuration, 0, pwfx, NULL);
    if (FAILED(hr)) {
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioCaptureClient* pCaptureClient = nullptr;
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr) || !pCaptureClient) {
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        pCaptureClient->Release();
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    std::vector<float> mono_buffer;
    mono_buffer.reserve(4096);

    while (m_running) {
        UINT32 packetLength = 0;
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            Sleep(5);
            continue;
        }

        if (packetLength == 0) {
            Sleep(5);
            continue;
        }

        while (packetLength > 0) {
            BYTE* pData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            hr = pCaptureClient->GetBuffer(&pData, &numFrames, &flags, NULL, NULL);
            if (FAILED(hr)) break;

            mono_buffer.resize(numFrames);

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                std::fill(mono_buffer.begin(), mono_buffer.end(), 0.0f);
            } else if (is_float && m_bits_per_sample == 32) {
                const float* floatData = reinterpret_cast<const float*>(pData);
                for (UINT32 f = 0; f < numFrames; ++f) {
                    float sum = 0.0f;
                    for (int c = 0; c < m_channels; ++c) {
                        sum += floatData[f * m_channels + c];
                    }
                    mono_buffer[f] = sum / m_channels;
                }
            } else if (m_bits_per_sample == 16) {
                const int16_t* pcm16 = reinterpret_cast<const int16_t*>(pData);
                for (UINT32 f = 0; f < numFrames; ++f) {
                    float sum = 0.0f;
                    for (int c = 0; c < m_channels; ++c) {
                        sum += pcm16[f * m_channels + c] / 32768.0f;
                    }
                    mono_buffer[f] = sum / m_channels;
                }
            } else {
                std::fill(mono_buffer.begin(), mono_buffer.end(), 0.0f);
            }

            pCaptureClient->ReleaseBuffer(numFrames);

            if (m_callback && !mono_buffer.empty()) {
                m_callback(mono_buffer.data(), mono_buffer.size());
            }

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }
    }

    pAudioClient->Stop();
    pCaptureClient->Release();
    CoTaskMemFree(pwfx);
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
}
