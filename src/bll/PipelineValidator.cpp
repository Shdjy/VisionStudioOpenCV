#include "PipelineValidator.h"
#include "../core/OpRegistry.h"
#include "../utils/Logger.h"
#include <sstream>

namespace
{
double GetNumericLimit(const std::variant<int, double>& value)
{
    if (std::holds_alternative<int>(value))
    {
        return static_cast<double>(std::get<int>(value));
    }
    return std::get<double>(value);
}
}

std::vector<ValidationIssue> PipelineValidator::ValidateStep(
    const PipelineStep& step,
    int stepIndex,
    int branchIndex,
    int incomingChannels)
{
    std::vector<ValidationIssue> issues;
    IOpNode* op = step.GetOpNode();
    if (!op)
    {
        ValidationIssue issue;
        issue.Severity = ValidationSeverity::Error;
        issue.BranchIndex = branchIndex;
        issue.StepIndex = stepIndex;
        issue.Message = "步骤中包含空算子实例";
        issue.Suggestion = "请重新添加有效算子节点";
        issues.push_back(issue);
        return issues;
    }

    std::string opName = op->GetName();

    // 1. 检查各参数范围与枚举合法性
    for (const auto& param : op->GetParameters())
    {
        switch (param.Type)
        {
            case ParamType::IntSlider:
            {
                int val = param.GetInt();
                double minVal = GetNumericLimit(param.MinValue);
                double maxVal = GetNumericLimit(param.MaxValue);
                if (val < minVal || val > maxVal)
                {
                    ValidationIssue issue;
                    issue.Severity = ValidationSeverity::Error;
                    issue.BranchIndex = branchIndex;
                    issue.StepIndex = stepIndex;
                    issue.OperatorName = opName;
                    issue.ParameterName = param.Name;
                    issue.Message = "整数参数 " + param.DisplayName + " 值 (" + std::to_string(val) +
                                    ") 超出合法范围 [" + std::to_string(static_cast<int>(minVal)) + ", " +
                                    std::to_string(static_cast<int>(maxVal)) + "]";
                    issue.Suggestion = "请将该参数调整至合法区间内";
                    issues.push_back(issue);
                }

                // 卷积核必须为奇数检查
                if ((param.Name == "ksize" || param.Name == "kernelSize") && (val % 2 == 0))
                {
                    ValidationIssue issue;
                    issue.Severity = ValidationSeverity::Error;
                    issue.BranchIndex = branchIndex;
                    issue.StepIndex = stepIndex;
                    issue.OperatorName = opName;
                    issue.ParameterName = param.Name;
                    issue.Message = "核大小必须为正奇数 (当前为 " + std::to_string(val) + ")";
                    issue.Suggestion = "请将核大小改为奇数，如 " + std::to_string(val + 1);
                    issues.push_back(issue);
                }
                break;
            }
            case ParamType::DoubleSlider:
            {
                double val = param.GetDouble();
                double minVal = GetNumericLimit(param.MinValue);
                double maxVal = GetNumericLimit(param.MaxValue);
                if (val < minVal - 1e-5 || val > maxVal + 1e-5)
                {
                    ValidationIssue issue;
                    issue.Severity = ValidationSeverity::Error;
                    issue.BranchIndex = branchIndex;
                    issue.StepIndex = stepIndex;
                    issue.OperatorName = opName;
                    issue.ParameterName = param.Name;
                    issue.Message = "浮点参数 " + param.DisplayName + " 值超出范围 [" +
                                    std::to_string(minVal) + ", " + std::to_string(maxVal) + "]";
                    issue.Suggestion = "请将参数限制在指定范围内";
                    issues.push_back(issue);
                }
                break;
            }
            case ParamType::ComboBox:
            {
                int val = param.GetInt();
                if (val < 0 || val >= static_cast<int>(param.EnumOptions.size()))
                {
                    ValidationIssue issue;
                    issue.Severity = ValidationSeverity::Error;
                    issue.BranchIndex = branchIndex;
                    issue.StepIndex = stepIndex;
                    issue.OperatorName = opName;
                    issue.ParameterName = param.Name;
                    issue.Message = "枚举参数索引 (" + std::to_string(val) + ") 超出选项范围";
                    issue.Suggestion = "请从可用下拉列表中选择有效项";
                    issues.push_back(issue);
                }
                break;
            }
            default:
                break;
        }
    }

    // 2. 通道与前置数据契约检查
    if (incomingChannels > 0)
    {
        // 需要单通道灰度图输入的算子分类
        if (op->GetCategory() == OpCategory::Thresholding ||
            op->GetCategory() == OpCategory::ContourAnalysis ||
            opName == "Canny" ||
            opName == "Sobel" ||
            opName == "Laplacian" ||
            opName == "DistanceTransform" ||
            opName == "ConnectedComponents")
        {
            if (incomingChannels != 1)
            {
                ValidationIssue issue;
                issue.Severity = ValidationSeverity::Warning;
                issue.BranchIndex = branchIndex;
                issue.StepIndex = stepIndex;
                issue.OperatorName = opName;
                issue.Message = "算子 " + op->GetDisplayName() + " 期望单通道灰度输入，但上游输入通道数为 " +
                                std::to_string(incomingChannels);
                issue.Suggestion = "建议在当前算子前插入“色彩空间转换 (CvtColor)”将图像转换为灰度图";
                issues.push_back(issue);
            }
        }
    }

    return issues;
}

PipelineValidationReport PipelineValidator::Validate(const PipelineManager& pipeline, const cv::Mat& sourceImage)
{
    PipelineValidationReport report;
    report.IsValid = true;
    report.ErrorCount = 0;
    report.WarningCount = 0;

    int branchCount = pipeline.GetBranchCount();
    if (branchCount == 0)
    {
        ValidationIssue issue;
        issue.Severity = ValidationSeverity::Warning;
        issue.StepIndex = -1;
        issue.Message = "流水线为空，未创建任何分支";
        issue.Suggestion = "请添加算子或分支";
        report.Issues.push_back(issue);
        report.WarningCount++;
        report.Summary = "流水线为空";
        return report;
    }

    int currentChannels = sourceImage.empty() ? 0 : sourceImage.channels();

    for (int b = 0; b < branchCount; ++b)
    {
        int stepCount = pipeline.GetBranchStepCount(b);
        int branchInChannels = currentChannels;

        // 若该分支有专用源图
        const cv::Mat& bSrc = pipeline.GetBranchSourceImage(b);
        if (!bSrc.empty())
        {
            branchInChannels = bSrc.channels();
        }

        int trackedChannels = branchInChannels;

        for (int i = 0; i < stepCount; ++i)
        {
            const PipelineStep* step = pipeline.GetBranchStep(b, i);
            if (!step)
            {
                continue;
            }

            if (!step->IsEnabled())
            {
                continue;
            }

            auto stepIssues = ValidateStep(*step, i, b, trackedChannels);
            for (auto& issue : stepIssues)
            {
                if (issue.Severity == ValidationSeverity::Error)
                {
                    report.ErrorCount++;
                    report.IsValid = false;
                }
                else if (issue.Severity == ValidationSeverity::Warning)
                {
                    report.WarningCount++;
                }
                report.Issues.push_back(std::move(issue));
            }

            // 预测输出通道用于后续校验
            IOpNode* node = step->GetOpNode();
            if (node)
            {
                if (node->GetCategory() == OpCategory::ColorAndChannels)
                {
                    // 若是转灰度
                    if (node->GetName() == "CvtColor")
                    {
                        auto* p = node->FindParameter("code");
                        if (p && (p->GetInt() == 0 || p->GetInt() == 1)) // BGR2GRAY
                        {
                            trackedChannels = 1;
                        }
                        else
                        {
                            trackedChannels = 3;
                        }
                    }
                }
                else if (node->GetCategory() == OpCategory::Thresholding)
                {
                    trackedChannels = 1; // 二值图为单通道
                }
            }
        }
    }

    std::ostringstream oss;
    if (report.IsValid)
    {
        if (report.WarningCount > 0)
        {
            oss << "流水线静态校验通过，但有 " << report.WarningCount << " 个提示警告。";
        }
        else
        {
            oss << "流水线静态校验全部通过，无错误。";
        }
    }
    else
    {
        oss << "流水线校验失败，发现 " << report.ErrorCount << " 个错误，"
            << report.WarningCount << " 个警告。请根据建议修正。";
    }
    report.Summary = oss.str();

    return report;
}
