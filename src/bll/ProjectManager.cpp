#include "ProjectManager.h"
#include "../core/OpRegistry.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool ProjectManager::SaveProject(const PipelineManager& pipeline, const QString& filePath, QString& errorMsg)
{
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
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

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
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

    QJsonObject rootObj = doc.object();
    QJsonArray stepsArray = rootObj["pipeline"].toArray();

    pipeline.Clear();

    for (const auto& val : stepsArray)
    {
        QJsonObject stepObj = val.toObject();
        std::string opName = stepObj["name"].toString().toStdString();
        bool enabled = stepObj["enabled"].toBool(true);

        auto op = OpRegistry::Instance().Create(opName);
        if (!op)
        {
            continue;
        }

        QJsonObject paramsObj = stepObj["parameters"].toObject();
        for (auto& p : op->GetParameters())
        {
            QString pName = QString::fromStdString(p.Name);
            if (paramsObj.contains(pName))
            {
                if (p.Type == ParamType::IntSlider || p.Type == ParamType::ComboBox)
                {
                    p.Value = paramsObj[pName].toInt();
                }
                else if (p.Type == ParamType::DoubleSlider)
                {
                    p.Value = paramsObj[pName].toDouble();
                }
                else if (p.Type == ParamType::CheckBox)
                {
                    p.Value = paramsObj[pName].toBool();
                }
            }
        }

        int stepIdx = pipeline.AddStep(std::move(op));
        PipelineStep* addedStep = pipeline.GetStep(stepIdx);
        if (addedStep)
        {
            addedStep->SetEnabled(enabled);
        }
    }

    pipeline.Execute(true);
    return true;
}
