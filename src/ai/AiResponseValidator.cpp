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

    Logger::Info("AiResponseValidator", "收到大模型原始响应全文 (共 " +
                 std::to_string(rawResponse.length()) + " 字符):\n" + rawResponse.toStdString());

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
        plan.AnalysisMarkdown = "AI 已为您规划出如下视觉方案与算子配置：";
    }

    // ========================================================
    // 4. 若提取到了 JSON，尝试结构化解析
    // ========================================================
    QJsonObject root;
    QJsonArray stepsArr;
    bool hasValidJsonDoc = false;

    if (!jsonStr.isEmpty())
    {
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseErr);
        if (parseErr.error == QJsonParseError::NoError)
        {
            hasValidJsonDoc = true;
            if (doc.isObject())
            {
                root = doc.object();
            }
            else if (doc.isArray())
            {
                stepsArr = doc.array();
            }
        }
    }

    if (hasValidJsonDoc)
    {
        // 4.1 检测任务提取 (DetectionTask)
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

        // 4.2 流水线步骤提取 (Pipeline Steps)
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

                // 算子自省与大小写容错
                bool isRegistered = OpRegistry::Instance().Has(opName);
                if (!isRegistered)
                {
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
    }

    // ========================================================
    // 5. 智能兜底：文本算子扫描推导提取器 (Text-to-Pipeline Fallback)
    //    当模型仅以自然语言陈述推荐算子而漏掉 ```json 代码块时，
    //    自动扫描正文中提到的核心 OpenCV 算子，组装为流水线方案！
    // ========================================================
    if (!plan.HasPipeline && !plan.HasDetectionTask)
    {
        auto allRegisteredOps = OpRegistry::Instance().GetAllOperators();
        struct OpOccurrence {
            int pos;
            std::string opName;
        };
        std::vector<OpOccurrence> foundOps;

        QString scanText = remainingAfterThink.toLower();

        // 核心常用工业流水线算子库词典 (英中双语匹配)
        static const std::vector<std::pair<QString, std::string>> opKeywords = {
            {"gaussianblur", "GaussianBlur"}, {"高斯滤波", "GaussianBlur"}, {"高斯平滑", "GaussianBlur"},
            {"medianblur", "MedianBlur"}, {"中值滤波", "MedianBlur"},
            {"threshold", "Threshold"}, {"二值化", "Threshold"}, {"阈值分割", "Threshold"},
            {"canny", "Canny"}, {"边缘检测", "Canny"},
            {"sobel", "Sobel"},
            {"dilate", "Dilate"}, {"膨胀", "Dilate"},
            {"erode", "Erode"}, {"腐蚀", "Erode"},
            {"morphology", "Morphology"}, {"形态学", "Morphology"},
            {"contour", "FindContours"}, {"轮廓", "FindContours"},
            {"bilateral", "BilateralFilter"}, {"双边滤波", "BilateralFilter"},
            {"equalize", "EqualizeHist"}, {"直方图均衡", "EqualizeHist"}
        };

        for (const auto& kw : opKeywords)
        {
            int idx = scanText.indexOf(kw.first);
            if (idx >= 0 && OpRegistry::Instance().Has(kw.second))
            {
                // 检查是否已经添加过相同算子
                bool exists = false;
                for (const auto& item : foundOps)
                {
                    if (item.opName == kw.second)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    foundOps.push_back({idx, kw.second});
                }
            }
        }

        // 按在文本中出现的先后顺序排序
        std::sort(foundOps.begin(), foundOps.end(), [](const OpOccurrence& a, const OpOccurrence& b) {
            return a.pos < b.pos;
        });

        if (!foundOps.empty())
        {
            QJsonArray fallbackSteps;
            for (const auto& item : foundOps)
            {
                QJsonObject stepObj;
                stepObj["name"] = QString::fromStdString(item.opName);
                QJsonObject params;
                if (item.opName == "GaussianBlur")
                {
                    params["ksize"] = 5;
                    params["sigmaX"] = 1.0;
                }
                else if (item.opName == "Threshold")
                {
                    params["thresh"] = 128;
                    params["maxval"] = 255;
                    params["type"] = 0;
                }
                else if (item.opName == "Canny")
                {
                    params["threshold1"] = 50;
                    params["threshold2"] = 150;
                }
                stepObj["parameters"] = params;
                fallbackSteps.append(stepObj);
            }

            plan.PipelineSteps = fallbackSteps;
            plan.HasPipeline = true;
            Logger::Info("AiResponseValidator", "通过智能文本扫描兜底推导出 " +
                         std::to_string(fallbackSteps.size()) + " 个流水线算子");

            plan.AnalysisMarkdown += "\n\n> 💡 **已根据 AI 分析方案自动推导出推荐算子流水线**，可点击下方【📋 载入流水线】按钮一键应用！";
        }
    }

    plan.IsValid = true;
    return plan;
}
