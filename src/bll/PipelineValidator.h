#pragma once

#include "PipelineManager.h"
#include "../core/OperatorMetadata.h"
#include <string>
#include <vector>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 校验问题严重级别
 */
enum class ValidationSeverity
{
    Info,       // 提示信息
    Warning,    // 警告 (可能导致非预期效果，但可尝试执行)
    Error       // 致命错误 (无法执行，例如参数越界、缺少依赖)
};

/**
 * @brief 单个校验问题项
 */
struct ValidationIssue
{
    ValidationSeverity Severity = ValidationSeverity::Error;
    int BranchIndex = 0;                        // 分支索引
    int StepIndex = -1;                         // 步骤索引 (-1 代表流水线级问题)
    std::string OperatorName;                   // 算子名
    std::string ParameterName;                  // 关联参数名
    std::string Message;                        // 问题原因描述
    std::string Suggestion;                     // 修复建议

    std::string SeverityToString() const
    {
        switch (Severity)
        {
            case ValidationSeverity::Info:
                return "提示";
            case ValidationSeverity::Warning:
                return "警告";
            case ValidationSeverity::Error:
            default:
                return "错误";
        }
    }

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["severity"] = QString::fromStdString(SeverityToString());
        obj["branchIndex"] = BranchIndex;
        obj["stepIndex"] = StepIndex;
        obj["operatorName"] = QString::fromStdString(OperatorName);
        obj["parameterName"] = QString::fromStdString(ParameterName);
        obj["message"] = QString::fromStdString(Message);
        obj["suggestion"] = QString::fromStdString(Suggestion);
        return obj;
    }
};

/**
 * @brief 流水线完整静态校验报告
 */
struct PipelineValidationReport
{
    bool IsValid = true;                        // 是否通过校验 (无 Error 级别问题即为 true)
    int ErrorCount = 0;                         // 错误数量
    int WarningCount = 0;                       // 警告数量
    std::vector<ValidationIssue> Issues;        // 问题列表
    std::string Summary;                        // 综合总结

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["isValid"] = IsValid;
        obj["errorCount"] = ErrorCount;
        obj["warningCount"] = WarningCount;
        obj["summary"] = QString::fromStdString(Summary);

        QJsonArray arr;
        for (const auto& issue : Issues)
        {
            arr.append(issue.ToJson());
        }
        obj["issues"] = arr;
        return obj;
    }
};

/**
 * @brief 工业视觉流水线静态校验器
 * 
 * 负责在流水线执行前执行多维静态与契约校验：
 * 1. 算子可用性与生命周期校验
 * 2. 参数类型、边界范围与枚举有效性校验
 * 3. 输入输出数据类型与色彩通道 (BGR vs 灰度) 兼容性契约校验
 * 4. 流程拓扑与前置依赖检查
 */
class PipelineValidator
{
public:
    /**
     * @brief 对整条流水线的所有分支与步骤进行全面静态校验
     * @param pipeline 流水线管理器引用
     * @param sourceImage 基准输入图像 (可选，用于通道匹配校验)
     * @return 校验报告
     */
    static PipelineValidationReport Validate(const PipelineManager& pipeline, const cv::Mat& sourceImage = cv::Mat());

    /**
     * @brief 校验单个步骤算子与参数
     * @param step 流水线步骤
     * @param stepIndex 步骤索引
     * @param branchIndex 分支索引
     * @param incomingChannels 预测输入通道数 (0 表示未知)
     * @return 问题列表
     */
    static std::vector<ValidationIssue> ValidateStep(
        const PipelineStep& step,
        int stepIndex,
        int branchIndex = 0,
        int incomingChannels = 0);
};
