#include "AiResponseValidator.h"
#include "../core/OpRegistry.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QRegularExpression>

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

    QString jsonStr = ExtractJsonBlock(rawResponse);
    plan.RawJsonString = jsonStr;

    // 提取 Markdown 分析文本 (去掉代码块)
    QString markdown = rawResponse;
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

    // 1. 判断是否为检测任务 (DetectionTask)
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
        Logger::Info("AiResponseValidator", "AI 检测任务校验成功: " + plan.Task.Name);
    }
    // 2. 判断是否为流水线 (Pipeline)
    else if (root.contains("pipeline") && root["pipeline"].isArray())
    {
        QJsonArray stepsArr = root["pipeline"].toArray();
        QJsonArray validatedSteps;

        for (int i = 0; i < stepsArr.size(); ++i)
        {
            QJsonObject sObj = stepsArr[i].toObject();
            std::string opName = sObj["name"].toString().toStdString();

            // 检查算子白名单
            auto opInstance = OpRegistry::Instance().Create(opName);
            if (!opInstance)
            {
                plan.IsValid = false;
                plan.ErrorMessage = QString("AI 推荐了本地未支持的未知算子: %1").arg(QString::fromStdString(opName));
                Logger::Warn("AiResponseValidator", plan.ErrorMessage.toStdString());
                return plan;
            }

            validatedSteps.append(sObj);
        }

        plan.HasPipeline = true;
        plan.PipelineSteps = validatedSteps;
        plan.IsValid = true;
        Logger::Info("AiResponseValidator", "AI 流水线校验成功，步骤数: " + std::to_string(validatedSteps.size()));
    }
    else
    {
        plan.IsValid = false;
        plan.ErrorMessage = "JSON 配置既不包含 taskType 也缺少 pipeline 步骤数组";
    }

    return plan;
}
