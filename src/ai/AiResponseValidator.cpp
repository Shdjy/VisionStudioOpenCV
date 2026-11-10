#include "AiResponseValidator.h"
#include "../core/OpRegistry.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDateTime>
#include <algorithm>

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
    // 1. 优先匹配标准的 Markdown 代码块 ```json ... ``` 或 ``` ... ```
    QRegularExpression regex("```(?:json)?\\s*([\\s\\S]*?)\\s*```", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(text);
    if (match.hasMatch())
    {
        QString block = match.captured(1).trimmed();
        if ((block.startsWith('{') && block.endsWith('}')) || (block.startsWith('[') && block.endsWith(']')))
        {
            return block;
        }
    }

    // 2. 寻找最外层大括号对象 { ... }
    int startObj = text.indexOf('{');
    int endObj = text.lastIndexOf('}');
    if (startObj != -1 && endObj != -1 && endObj > startObj)
    {
        return text.mid(startObj, endObj - startObj + 1).trimmed();
    }

    // 3. 寻找最外层数组 [ ... ]
    int startArr = text.indexOf('[');
    int endArr = text.lastIndexOf(']');
    if (startArr != -1 && endArr != -1 && endArr > startArr)
    {
        return text.mid(startArr, endArr - startArr + 1).trimmed();
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

    Logger::Info("AiResponseValidator", "开始解析 AI 响应全文 (共 " +
                 std::to_string(rawResponse.length()) + " 字符)");

    // 1. 提取推理与思维链 (<think>...</think>)
    QString remainingAfterThink;
    plan.ThinkingProcess = ExtractThinkingBlock(rawResponse, remainingAfterThink);

    // 2. 提取 JSON 配置代码块
    QString jsonStr = ExtractJsonBlock(remainingAfterThink);
    plan.RawJsonString = jsonStr;

    // 3. 提取 Markdown 分析正文 (去除代码块)
    QString markdown = remainingAfterThink;
    QRegularExpression codeBlockRegex("```(?:json)?[\\s\\S]*?```", QRegularExpression::CaseInsensitiveOption);
    markdown.remove(codeBlockRegex);
    plan.AnalysisMarkdown = markdown.trimmed();

    if (plan.AnalysisMarkdown.isEmpty())
    {
        // 如果去除代码块后没有文字，则使用未处理的 Markdown 作为说明
        plan.AnalysisMarkdown = "AI 已为您推演出视觉方案与可执行配置：";
    }

    // 如果未找到任何 JSON 代码块，但有丰富的自然语言分析内容
    if (jsonStr.isEmpty())
    {
        Logger::Info("AiResponseValidator", "未提取到 JSON 代码块，按纯方案分析问答呈现");
        plan.IsValid = true;
        plan.HasDetectionTask = false;
        plan.HasPipeline = false;
        return plan;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError)
    {
        Logger::Warn("AiResponseValidator", "JSON 解析语法警告: " + parseErr.errorString().toStdString() +
                     "，尝试作为自然语言方案呈现");
        // 容错：语法有轻微问题时不报错中断会话，依然让用户看到 Markdown
        plan.IsValid = true;
        plan.HasDetectionTask = false;
        plan.HasPipeline = false;
        return plan;
    }

    QJsonObject root;
    QJsonArray stepsArr;

    if (doc.isObject())
    {
        root = doc.object();
    }
    else if (doc.isArray())
    {
        stepsArr = doc.array();
    }

    // ========================================================
    // 4. 智能检测任务提取 (DetectionTask) - 支持多种变体与嵌套结构
    // ========================================================
    QJsonObject taskObj;
    if (root.contains("taskType") || root.contains("task_type") || root.contains("type"))
    {
        taskObj = root;
    }
    else if (root.contains("task") && root["task"].isObject())
    {
        taskObj = root["task"].toObject();
    }
    else if (root.contains("detectionTask") && root["detectionTask"].isObject())
    {
        taskObj = root["detectionTask"].toObject();
    }
    else if (root.contains("detection_task") && root["detection_task"].isObject())
    {
        taskObj = root["detection_task"].toObject();
    }
    else if (root.contains("inspectionTask") && root["inspectionTask"].isObject())
    {
        taskObj = root["inspectionTask"].toObject();
    }

    if (!taskObj.isEmpty())
    {
        QString rawTypeStr;
        if (taskObj.contains("taskType")) rawTypeStr = taskObj["taskType"].toString();
        else if (taskObj.contains("task_type")) rawTypeStr = taskObj["task_type"].toString();
        else if (taskObj.contains("type")) rawTypeStr = taskObj["type"].toString();

        std::string typeStr = rawTypeStr.toStdString();
        plan.Task.Id = "ai_task_" + QString::number(QDateTime::currentMSecsSinceEpoch()).toStdString();
        plan.Task.Name = taskObj["name"].toString("AI 生成视觉检测任务").toStdString();
        plan.Task.Type = StringToDetectionTaskType(typeStr);
        plan.Task.Enabled = true;

        if (taskObj.contains("configuration") && taskObj["configuration"].isObject())
        {
            plan.Task.Configuration = taskObj["configuration"].toObject();
        }
        else if (taskObj.contains("config") && taskObj["config"].isObject())
        {
            plan.Task.Configuration = taskObj["config"].toObject();
        }
        else if (taskObj.contains("parameters") && taskObj["parameters"].isObject())
        {
            plan.Task.Configuration = taskObj["parameters"].toObject();
        }

        if (taskObj.contains("rules") && taskObj["rules"].isArray())
        {
            plan.Task.Rules = taskObj["rules"].toArray();
        }

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
        Logger::Info("AiResponseValidator", "成功提取检测任务: " + plan.Task.Name);
    }

    // ========================================================
    // 5. 智能流水线步骤提取 (Pipeline Steps) - 兼容 pipeline / steps / operators
    // ========================================================
    if (stepsArr.isEmpty())
    {
        if (root.contains("pipeline") && root["pipeline"].isArray())
        {
            stepsArr = root["pipeline"].toArray();
        }
        else if (root.contains("steps") && root["steps"].isArray())
        {
            stepsArr = root["steps"].toArray();
        }
        else if (root.contains("pipelineSteps") && root["pipelineSteps"].isArray())
        {
            stepsArr = root["pipelineSteps"].toArray();
        }
        else if (root.contains("operators") && root["operators"].isArray())
        {
            stepsArr = root["operators"].toArray();
        }
        else if (root.contains("pipeline") && root["pipeline"].isObject())
        {
            QJsonObject pObj = root["pipeline"].toObject();
            if (pObj.contains("steps") && pObj["steps"].isArray())
            {
                stepsArr = pObj["steps"].toArray();
            }
        }
    }

    if (!stepsArr.isEmpty())
    {
        QJsonArray validSteps;
        for (const auto& stepVal : stepsArr)
        {
            if (!stepVal.isObject())
            {
                continue;
            }
            QJsonObject stepObj = stepVal.toObject();

            QString rawOpName;
            if (stepObj.contains("name")) rawOpName = stepObj["name"].toString();
            else if (stepObj.contains("operator")) rawOpName = stepObj["operator"].toString();
            else if (stepObj.contains("op")) rawOpName = stepObj["op"].toString();

            std::string opName = rawOpName.trimmed().toStdString();
            if (opName.empty())
            {
                continue;
            }

            // 算子名称存在性自省校验与大小写容错
            bool isRegistered = OpRegistry::Instance().Has(opName);
            if (!isRegistered)
            {
                // 尝试大小写不敏感匹配
                auto allOps = OpRegistry::Instance().GetAllOperators();
                for (const auto& m : allOps)
                {
                    if (QString::fromStdString(m.Name).compare(rawOpName, Qt::CaseInsensitive) == 0)
                    {
                        opName = m.Name;
                        isRegistered = true;
                        stepObj["name"] = QString::fromStdString(m.Name);
                        break;
                    }
                }
            }

            if (!isRegistered)
            {
                Logger::Warn("AiResponseValidator", "拦截未注册算子: " + opName);
                continue;
            }

            // 参数键名标准化
            if (!stepObj.contains("parameters") && stepObj.contains("params") && stepObj["params"].isObject())
            {
                stepObj["parameters"] = stepObj["params"].toObject();
            }

            validSteps.append(stepObj);
        }

        if (!validSteps.isEmpty())
        {
            plan.PipelineSteps = validSteps;
            plan.HasPipeline = true;
            Logger::Info("AiResponseValidator", "成功提取流水线步骤: 共 " +
                         std::to_string(validSteps.size()) + " 个合法算子");
        }
    }

    // 方案整体判定：无论是生成了任务、流水线，还是给出了专业分析解答，均视为成功规划！
    plan.IsValid = true;
    return plan;
}
