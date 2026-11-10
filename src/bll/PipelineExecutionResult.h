#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 单个算子步骤执行指标
 */
struct StepExecutionMetric
{
    int StepIndex = 0;                          // 步骤索引 (从 0 开始)
    std::string OperatorName;                   // 算子英文名
    std::string OperatorDisplayName;            // 算子中文名
    double DurationMs = 0.0;                    // 耗时 (毫秒)
    cv::Size InputSize;                         // 输入图像尺寸
    int InputChannels = 0;                      // 输入通道数
    cv::Size OutputSize;                        // 输出图像尺寸
    int OutputChannels = 0;                     // 输出通道数
    bool Success = true;                        // 是否执行成功
    std::string ErrorMessage;                   // 失败异常详情

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["stepIndex"] = StepIndex;
        obj["operatorName"] = QString::fromStdString(OperatorName);
        obj["operatorDisplayName"] = QString::fromStdString(OperatorDisplayName);
        obj["durationMs"] = DurationMs;
        obj["inputWidth"] = InputSize.width;
        obj["inputHeight"] = InputSize.height;
        obj["inputChannels"] = InputChannels;
        obj["outputWidth"] = OutputSize.width;
        obj["outputHeight"] = OutputSize.height;
        obj["outputChannels"] = OutputChannels;
        obj["success"] = Success;
        obj["errorMessage"] = QString::fromStdString(ErrorMessage);
        return obj;
    }
};

/**
 * @brief 整条流水线执行汇总与统计结果
 */
struct PipelineExecutionSummary
{
    bool Success = true;                        // 整条流水线是否成功
    double TotalDurationMs = 0.0;               // 总耗时 (毫秒)
    int TotalSteps = 0;                         // 总步骤数
    int ExecutedSteps = 0;                      // 实际执行步骤数
    int SkippedSteps = 0;                       // 禁用跳过步骤数
    int FailedStepIndex = -1;                   // 失败步骤索引 (-1 表示无失败)
    std::string ErrorMessage;                   // 错误详情
    std::vector<StepExecutionMetric> StepMetrics; // 各步骤耗时明细

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["success"] = Success;
        obj["totalDurationMs"] = TotalDurationMs;
        obj["totalSteps"] = TotalSteps;
        obj["executedSteps"] = ExecutedSteps;
        obj["skippedSteps"] = SkippedSteps;
        obj["failedStepIndex"] = FailedStepIndex;
        obj["errorMessage"] = QString::fromStdString(ErrorMessage);

        QJsonArray stepsArr;
        for (const auto& metric : StepMetrics)
        {
            stepsArr.append(metric.ToJson());
        }
        obj["steps"] = stepsArr;
        return obj;
    }
};
