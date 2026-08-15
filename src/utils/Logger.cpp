#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger()
{
}

void Logger::Log(LogLevel level, const std::string& module, const std::string& message)
{
    std::lock_guard<std::mutex> lock(_mutex);

    LogEntry entry;
    entry.Timestamp = GetCurrentTimestamp();
    entry.Level = level;
    entry.Module = module;
    entry.Message = message;

    // 输出到标准控制台
    std::string levelStr = LevelToString(level);
    std::cout << "[" << entry.Timestamp << "] [" << levelStr << "] [" << module << "] " << message << std::endl;

    // 触发所有 UI 注册的回调
    for (const auto& callback : _callbacks)
    {
        if (callback)
        {
            callback(entry);
        }
    }
}

void Logger::Debug(const std::string& module, const std::string& message)
{
    Instance().Log(LogLevel::Debug, module, message);
}

void Logger::Info(const std::string& module, const std::string& message)
{
    Instance().Log(LogLevel::Info, module, message);
}

void Logger::Warn(const std::string& module, const std::string& message)
{
    Instance().Log(LogLevel::Warning, module, message);
}

void Logger::Error(const std::string& module, const std::string& message)
{
    Instance().Log(LogLevel::Error, module, message);
}

void Logger::RegisterCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks.push_back(callback);
}

std::string Logger::LevelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::GetCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    struct tm buf;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    ss << std::put_time(&buf, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}
