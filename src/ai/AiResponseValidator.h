#pragma once

#include "../detection/DetectionTask.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief AI 生成内容校验与解析结果结构
 */
struct ValidatedAiPlan
{
    bool IsValid = false;                       // 整体解析与本地校验是否通过
    QString ErrorMessage;                       // 校验失败原因
    QString AnalysisMarkdown;                   // AI 输出的自然语言分析说明 (供用户阅读)
    bool HasDetectionTask = false;              // 是否包含检测任务配置
    DetectionTask Task;                         // 解析出的检测任务
    bool HasPipeline = false;                   // 是否包含流水线步骤配置
    QJsonArray PipelineSteps;                   // 流水线步骤
    QString RawJsonString;                      // 提取出的原始 JSON 字符串
};

/**
 * @brief AI 响应安全校验器 (白名单校验与结构化解析)
 * 
 * 严格防范大模型幻觉：
 * 1. 确保所有被引用的算子 100% 存在于本地 OpRegistry
 * 2. 校验参数数值合法性、正奇数约束
 * 3. 校验检测任务结构规范
 */
class AiResponseValidator
{
public:
    /**
     * @brief 解析并严格校验 AI 输出文本
     * @param rawResponse AI 返回的完整 Markdown/JSON 文本
     * @return 校验解析结果
     */
    static ValidatedAiPlan ValidateAndParse(const QString& rawResponse);

private:
    static QString ExtractJsonBlock(const QString& text);
};
