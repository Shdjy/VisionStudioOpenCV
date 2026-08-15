#pragma once

#include <string>

/**
 * @brief 日志级别枚举
 */
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

/**
 * @brief 日志输出接口抽象类
 * 
 * 遵循依赖倒置原则（DIP），业务与UI层面向该接口记录日志，
 * 底层可实现控制台输出、文件记录或UI界面文本分发。
 */
class ILogger
{
public:
    virtual ~ILogger() = default;

    /**
     * @brief 写入日志
     * @param level 日志等级
     * @param module 来源模块名称
     * @param message 日志具体信息
     */
    virtual void Log(LogLevel level, const std::string& module, const std::string& message) = 0;
};
