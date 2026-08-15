#pragma once

#include "ILogger.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

/**
 * @brief 格式化日志条目
 */
struct LogEntry
{
    std::string Timestamp;
    LogLevel Level;
    std::string Module;
    std::string Message;
};

/**
 * @brief 统一日志中心（单例模式）
 * 
 * 管理所有日志输出通道（控制台、文件、UI回调等）。
 */
class Logger : public ILogger
{
public:
    using LogCallback = std::function<void(const LogEntry&)>;

    static Logger& Instance()
    {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 实现 ILogger 接口
     */
    void Log(LogLevel level, const std::string& module, const std::string& message) override;

    /**
     * @brief 便捷静态记录方法
     */
    static void Debug(const std::string& module, const std::string& message);
    static void Info(const std::string& module, const std::string& message);
    static void Warn(const std::string& module, const std::string& message);
    static void Error(const std::string& module, const std::string& message);

    /**
     * @brief 注册日志监听回调（例如用于 UI 显示）
     */
    void RegisterCallback(LogCallback callback);

    /**
     * @brief 级别转字符串
     */
    static std::string LevelToString(LogLevel level);

private:
    Logger();
    ~Logger() override = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string GetCurrentTimestamp() const;

    std::vector<LogCallback> _callbacks;
    std::mutex _mutex;
};
