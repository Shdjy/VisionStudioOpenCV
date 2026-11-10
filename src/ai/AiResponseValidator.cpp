#include "AiResponseValidator.h"
#include "../core/OpRegistry.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDateTime>

QString AiResponseValidator::ExtractThinkingBlock(const QString& text, QString& remainingText)
{
    remainingText = text;
    QRegularExpression thinkRegex("<think>([\\s\\S]*?)</think>", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = thinkRegex.match(text);
    if (match.hasMatch())
    {
        QString thinking = match.captured(1).trimmed();
        remainingText.remove(thinkRegex);
        remainingText = remainingText.trimmed();
        return thinking;
    }
    return QString();
}

QString AiResponseValidator::ExtractJsonBlock(const QString& text)
{
    // 匹配 ```json ... ``` 或 ``` ... ```
    QRegularExpression regex("```(?:json)?\\s*([\\s\\S]*?)\\s*```", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(text);
    if (match.hasMatch())
    {
        return match.captured(1).trimmed();
    }

    // 寻找最外层大括号
    int start = text.indexOf('{');
    int end = text.lastIndexOf('}');
    if (start != -1 && end != -1 && end > start)
    {
        return text.mid(start, end - start + 1).trimmed();
    }

    return QString();
}

ValidatedAiPlan AiResponseValidator::ValidateAndParse(const QString& rawResponse)
{
    ValidatedAiPlan plan;
    if (rawResponse.trimmed().isEmpty())
    {
        plan.IsValid = false;
        plan.ErrorMessage = "AI 响应内容为空";
        return plan;
    }

    // 1. 提取推理与思维链 (<think>...</think>)
    QString remainingAfterThink;
    plan.ThinkingProcess = ExtractThinkingBlock(rawResponse, remainingAfterThink);

    // 2. 提取 JSON 代码块
    QString jsonStr = ExtractJsonBlock(remainingAfterThink);
    plan.RawJsonString = jsonStr;

    // 3. 提取 Markdown 分析文本 (去除代码块)
    QString markdown = remainingAfterThink;
    QRegularExpression codeBlockRegex("```(?:json)?[\\s\\S]*?```", QRegularExpression::CaseInsensitiveOption);
    markdown.remove(codeBlockRegex);
    plan.AnalysisMarkdown = markdown.trimmed();

    if (jsonStr.isEmpty())
    {
        plan.IsValid = false;
        plan.ErrorMessage = "未在 AI 响应中提取到有效的 JSON 配置代码块";
        return plan;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
    {
        plan.IsValid = false;
        plan.ErrorMessage = "JSON 解析语法错误: " + parseErr.errorString();
        return plan;
    }

    QJsonObject root = doc.object();

    // 4. 判断是否为检测任务 (DetectionTask)
    if (root.contains("taskType"))
    {
        std::string typeStr = root["taskType"].toString().toStdString();
        plan.Task.Id = "ai_task_" + QString::number(QDateTime::currentMSecsSinceEpoch()).toStdString();
        plan.Task.Name = root["name"].toString("AI 生成检测任务").toStdString();
        plan.Task.Type = StringToDetectionTaskType(typeStr);
        plan.Task.Enabled = true;
        plan.Task.Configuration = root["configuration"].toObject();
        plan.Task.Rules = root["rules"].toArray();

        // 校验配置参数：核大小正奇数校验与修正
        QJsonObject cfg = plan.Task.Configuration;
        bool modified = false;
        for (auto it = cfg.begin(); it != cfg.end(); ++it)
        {
            QString key = it.key();
            if ((key.contains("kernel", Qt::CaseInsensitive) || key.contains("ksize", Qt::CaseInsensitive)) &&
                it.value().isDouble())
            {
                int k = it.value().toInt();
                if (k <= 0)
                {
                    k = 3;
                    modified = true;
                }
                else if (k % 2 == 0)
                {
                    k += 1;
                    modified = true;
                }
                cfg[key] = k;
            }
        }
        if (modified)
        {
            plan.Task.Configuration = cfg;
        }

        plan.HasDetectionTask = true;
        plan.IsValid = true;
    }
    // 5. 判断是否为流水线算子步骤 (steps)
    else if (root.contains("steps") && root["steps"].isArray())
    {
        QJsonArray steps = root["steps"].toArray();
        QJsonArray validSteps;

        for (const auto& stepVal : steps)
        {
            if (!stepVal.isObject())
            {
                continue;
            }
            QJsonObject stepObj = stepVal.toObject();
            std::string opName = stepObj["name"].toString().toStdString();

            // 严格白名单防御：必须已在系统 OpRegistry 中注册
            if (!OpRegistry::Instance().Has(opName))
            {
                Logger::Warn("AiResponseValidator", "拦截未注册幻觉算子: " + opName);
                continue;
            }

            validSteps.append(stepObj);
        }

        if (validSteps.isEmpty())
        {
            plan.IsValid = false;
            plan.ErrorMessage = "流水线所有算子均未通过系统合法性白名单校验";
            return plan;
        }

        plan.PipelineSteps = validSteps;
        plan.HasPipeline = true;
        plan.IsValid = true;
    }
    else
    {
        plan.IsValid = false;
        plan.ErrorMessage = "JSON 不包含 taskType 或 steps 节点，无法构造执行实体";
    }

    return plan;
}
