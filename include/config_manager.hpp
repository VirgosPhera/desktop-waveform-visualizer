#pragma once
#include "types.hpp"
#include <string>
#include <functional>
#include <thread>
#include <atomic>

class ConfigManager {
public:
    using ConfigChangedCallback = std::function<void(const AppConfig& config)>;

    ConfigManager();
    ~ConfigManager();

    bool LoadOrCreate(const std::string& file_path, AppConfig& out_config);
    bool Save(const std::string& file_path, const AppConfig& config);

    void StartWatcher(const std::string& file_path, ConfigChangedCallback callback);
    void StopWatcher();

private:
    void WatcherThread(std::string directory, std::string filename);

    std::atomic<bool> m_watching;
    std::thread m_thread;
    ConfigChangedCallback m_callback;
};
