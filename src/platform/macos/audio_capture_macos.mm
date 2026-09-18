#if defined(__APPLE__)
#include "audio_capture.hpp"
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>
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

// Audio queue callback for CoreAudio / BlackHole loopback
static void HandleInputBuffer(void* aqData, AudioQueueRef inAQ,
                              AudioQueueBufferRef inBuffer,
                              const AudioTimeStamp* inStartTime,
                              UInt32 inNumPackets,
                              const AudioStreamPacketDescription* inPacketDesc) {
    AudioCapture* capture = static_cast<AudioCapture*>(aqData);
    if (!capture || !capture->IsRunning()) return;

    const float* samples = static_cast<const float*>(inBuffer->mAudioData);
    size_t total_samples = inBuffer->mAudioDataByteSize / sizeof(float);
    size_t frames = total_samples / 2;

    std::vector<float> mono(frames);
    for (size_t i = 0; i < frames; ++i) {
        mono[i] = 0.5f * (samples[i * 2] + samples[i * 2 + 1]);
    }

    if (capture->m_callback) {
        capture->m_callback(mono.data(), mono.size());
    }

    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
}

void AudioCapture::CaptureThread() {
    AudioStreamBasicDescription format;
    memset(&format, 0, sizeof(format));
    format.mSampleRate = m_sample_rate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBytesPerPacket = 8;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 8;
    format.mChannelsPerFrame = 2;
    format.mBitsPerChannel = 32;

    AudioQueueRef queue = NULL;
    OSStatus status = AudioQueueNewInput(&format, HandleInputBuffer, this, NULL,
                                        kCFRunLoopCommonModes, 0, &queue);
    if (status != noErr) {
        std::cerr << "[AetherWave macOS] AudioQueueNewInput failed: " << status << std::endl;
        return;
    }

    const int kNumBuffers = 3;
    const int kBufferSize = 4096;
    AudioQueueBufferRef buffers[kNumBuffers];

    for (int i = 0; i < kNumBuffers; ++i) {
        AudioQueueAllocateBuffer(queue, kBufferSize, &buffers[i]);
        AudioQueueEnqueueBuffer(queue, buffers[i], 0, NULL);
    }

    AudioQueueStart(queue, NULL);

    while (m_running) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
    }

    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);
}
#endif
