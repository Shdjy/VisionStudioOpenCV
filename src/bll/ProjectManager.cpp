#include "ProjectManager.h"
#include "../core/OpRegistry.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

namespace
{
constexpr const char* ProjectVersion = "1.0";

bool ApplyJsonParameter(IOpNode& op, OpParameter& parameter, const QJsonValue& value)
{
    switch (parameter.Type)
    {
        case ParamType::IntSlider:
        case ParamType::ComboBox:
            return value.isDouble() && op.SetParameter(parameter.Name, value.toInt());
        case ParamType::DoubleSlider:
            return value.isDouble() && op.SetParameter(parameter.Name, value.toDouble());
        case ParamType::CheckBox:
            return value.isBool() && op.SetParameter(parameter.Name, value.toBool());
        case ParamType::FilePath:
            return value.isString() && op.SetParameter(parameter.Name, value.toString().toStdString());
    }
    return false;
}
}

bool ProjectManager::SaveProject(const PipelineManager& pipeline, const QString& filePath, QString& errorMsg)
{
    QJsonObject rootObj;
    rootObj["version"] = ProjectVersion;
    rootObj["appName"] = "VisionStudio OpenCV";

    QJsonArray stepsArray;
    for (int i = 0; i < pipeline.GetStepCount(); ++i)
    {
        const PipelineStep* step = pipeline.GetStep(i);
        if (!step || !step->GetOpNode())
        {
            continue;
        }

        IOpNode* op = step->GetOpNode();
        QJsonObject stepObj;
        stepObj["name"] = QString::fromStdString(op->GetName());
        stepObj["enabled"] = step->IsEnabled();

        QJsonObject paramsObj;
        for (const auto& p : op->GetParameters())
        {
            if (p.Type == ParamType::IntSlider || p.Type == ParamType::ComboBox)
            {
                paramsObj[QString::fromStdString(p.Name)] = p.GetInt();
            }
            else if (p.Type == ParamType::DoubleSlider)
            {
                paramsObj[QString::fromStdString(p.Name)] = p.GetDouble();
            }
            else if (p.Type == ParamType::CheckBox)
            {
                paramsObj[QString::fromStdString(p.Name)] = p.GetBool();
            }
            else if (p.Type == ParamType::FilePath)
            {
                paramsObj[QString::fromStdString(p.Name)] = QString::fromStdString(p.GetString());
            }
        }
        stepObj["parameters"] = paramsObj;
        stepsArray.append(stepObj);
    }

    rootObj["pipeline"] = stepsArray;

    QJsonDocument doc(rootObj);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        errorMsg = "无法打开文件以写入: " + file.errorString();
        return false;
    }

    const QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    if (file.write(jsonData) != jsonData.size())
    {
        errorMsg = "工程文件写入不完整: " + file.errorString();
        file.close();
        return false;
    }
    file.close();
    errorMsg.clear();
    return true;
}

bool ProjectManager::LoadProject(PipelineManager& pipeline, const QString& filePath, QString& errorMsg)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        errorMsg = "无法打开文件以读取: " + file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError)
    {
        errorMsg = "JSON 解析失败: " + parseErr.errorString();
        return false;
    }

    if (!doc.isObject())
    {
        errorMsg = "工程文件根节点必须是 JSON 对象";
        return false;
    }

    QJsonObject rootObj = doc.object();
    const QString version = rootObj["version"].toString();
    if (version != ProjectVersion)
    {
        errorMsg = "不支持的工程文件版本: " + version;
        return false;
    }
    if (!rootObj["pipeline"].isArray())
    {
        errorMsg = "工程文件缺少有效的 pipeline 数组";
        return false;
    }

    QJsonArray stepsArray = rootObj["pipeline"].toArray();
    struct LoadedStep
    {
        std::unique_ptr<IOpNode> Operator;
        bool Enabled = true;
    };
    std::vector<LoadedStep> loadedSteps;
    loadedSteps.reserve(static_cast<size_t>(stepsArray.size()));

    for (int stepIndex = 0; stepIndex < stepsArray.size(); ++stepIndex)
    {
        const QJsonValue val = stepsArray[stepIndex];
        if (!val.isObject())
        {
            errorMsg = QString("第 %1 个步骤不是有效对象").arg(stepIndex + 1);
            return false;
        }
        QJsonObject stepObj = val.toObject();
        std::string opName = stepObj["name"].toString().toStdString();
        bool enabled = stepObj["enabled"].toBool(true);

        auto op = OpRegistry::Instance().Create(opName);
        if (!op)
        {
            errorMsg = QString("第 %1 个步骤引用了未知算子: %2")
                .arg(stepIndex + 1)
                .arg(QString::fromStdString(opName));
            return false;
        }

        QJsonObject paramsObj = stepObj["parameters"].toObject();
        for (auto& p : op->GetParameters())
        {
            QString pName = QString::fromStdString(p.Name);
            if (paramsObj.contains(pName))
            {
                if (!ApplyJsonParameter(*op, p, paramsObj[pName]))
                {
                    errorMsg = QString("第 %1 个步骤的参数 %2 类型错误或超出范围")
                        .arg(stepIndex + 1)
                        .arg(pName);
                    return false;
                }
            }
        }

        loadedSteps.push_back({ std::move(op), enabled });
    }

    pipeline.Clear();
    for (auto& loadedStep : loadedSteps)
    {
        int stepIdx = pipeline.AddStep(std::move(loadedStep.Operator));
        PipelineStep* addedStep = pipeline.GetStep(stepIdx);
        if (addedStep)
        {
            addedStep->SetEnabled(loadedStep.Enabled);
        }
    }

    if (!pipeline.GetSourceImage().empty())
    {
        pipeline.Execute(true);
    }
    errorMsg.clear();
    return true;
}
