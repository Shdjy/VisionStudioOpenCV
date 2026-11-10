#include "ProjectManager.h"
#include "../core/OpRegistry.h"
#include "../detection/DetectionTaskManager.h"
#include "../utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

namespace
{
constexpr const char* ProjectVersionV1 = "1.0";
constexpr const char* ProjectVersionV2 = "2.0";

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

bool ProjectManager::SaveProject(
    const PipelineManager& pipeline,
    const DetectionTaskManager* taskManager,
    const QString& filePath,
    QString& errorMsg)
{
    QJsonObject rootObj;
    rootObj["version"] = ProjectVersionV2;
    rootObj["appName"] = "VisionStudio OpenCV";

    QJsonArray branchesArray;
    int branchCount = pipeline.GetBranchCount();
    for (int b = 0; b < branchCount; ++b)
    {
        QJsonObject branchObj;
        branchObj["name"] = QString::fromStdString(pipeline.GetBranchName(b));

        QJsonArray stepsArray;
        int stepCount = pipeline.GetBranchStepCount(b);
        for (int i = 0; i < stepCount; ++i)
        {
            const PipelineStep* step = pipeline.GetBranchStep(b, i);
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
        branchObj["steps"] = stepsArray;
        branchesArray.append(branchObj);

        // 兼容单分支格式
        if (b == 0)
        {
            rootObj["pipeline"] = stepsArray;
        }
    }

    rootObj["branches"] = branchesArray;

    // 序列化检测任务
    if (taskManager)
    {
        rootObj["detectionTasks"] = taskManager->ToJson();
    }

    QJsonDocument doc(rootObj);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        errorMsg = "无法打开文件以写入: " + file.errorString();
        Logger::Error("ProjectManager", errorMsg.toStdString());
        return false;
    }

    const QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    if (file.write(jsonData) != jsonData.size())
    {
        errorMsg = "工程文件写入不完整: " + file.errorString();
        file.close();
        Logger::Error("ProjectManager", errorMsg.toStdString());
        return false;
    }
    file.close();
    errorMsg.clear();
    Logger::Info("ProjectManager", "工程已成功保存至: " + filePath.toStdString());
    return true;
}

bool ProjectManager::SaveProject(const PipelineManager& pipeline, const QString& filePath, QString& errorMsg)
{
    return SaveProject(pipeline, nullptr, filePath, errorMsg);
}

bool ProjectManager::LoadProject(
    PipelineManager& pipeline,
    DetectionTaskManager* taskManager,
    const QString& filePath,
    QString& errorMsg)
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
    if (version != ProjectVersionV1 && version != ProjectVersionV2)
    {
        errorMsg = "不支持的工程文件版本: " + version + " (支持 1.0 与 2.0)";
        return false;
    }

    struct LoadedStep
    {
        std::unique_ptr<IOpNode> Operator;
        bool Enabled = true;
    };

    struct LoadedBranch
    {
        std::string Name;
        std::vector<LoadedStep> Steps;
    };

    std::vector<LoadedBranch> loadedBranches;

    if (rootObj.contains("branches") && rootObj["branches"].isArray())
    {
        QJsonArray branchesArray = rootObj["branches"].toArray();
        for (int b = 0; b < branchesArray.size(); ++b)
        {
            QJsonObject bObj = branchesArray[b].toObject();
            LoadedBranch lBranch;
            lBranch.Name = bObj["name"].toString().toStdString();
            if (lBranch.Name.empty())
            {
                lBranch.Name = (b == 0) ? "主流水线 (Main)" : ("分支 " + std::to_string(b));
            }

            QJsonArray stepsArray = bObj["steps"].toArray();
            for (int stepIndex = 0; stepIndex < stepsArray.size(); ++stepIndex)
            {
                QJsonObject stepObj = stepsArray[stepIndex].toObject();
                std::string opName = stepObj["name"].toString().toStdString();
                bool enabled = stepObj["enabled"].toBool(true);

                auto op = OpRegistry::Instance().Create(opName);
                if (!op)
                {
                    errorMsg = QString("分支 %1 步骤 %2 引用了未知算子: %3")
                                   .arg(QString::fromStdString(lBranch.Name))
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
                            errorMsg = QString("分支 %1 步骤 %2 参数 %3 错误")
                                           .arg(QString::fromStdString(lBranch.Name))
                                           .arg(stepIndex + 1)
                                           .arg(pName);
                            return false;
                        }
                    }
                }
                lBranch.Steps.push_back({ std::move(op), enabled });
            }
            loadedBranches.push_back(std::move(lBranch));
        }
    }
    else if (rootObj["pipeline"].isArray())
    {
        // 单分支兼容模式
        LoadedBranch mainBranch;
        mainBranch.Name = "主流水线 (Main)";
        QJsonArray stepsArray = rootObj["pipeline"].toArray();
        for (int stepIndex = 0; stepIndex < stepsArray.size(); ++stepIndex)
        {
            QJsonObject stepObj = stepsArray[stepIndex].toObject();
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
                        errorMsg = QString("第 %1 个步骤的参数 %2 错误").arg(stepIndex + 1).arg(pName);
                        return false;
                    }
                }
            }
            mainBranch.Steps.push_back({ std::move(op), enabled });
        }
        loadedBranches.push_back(std::move(mainBranch));
    }
    else
    {
        errorMsg = "工程文件缺少有效的 pipeline 或 branches 数组";
        return false;
    }

    // 重建分支结构
    while (pipeline.GetBranchCount() > 1)
    {
        pipeline.RemoveBranch(1);
    }
    pipeline.SetActiveBranchIndex(0);
    pipeline.Clear();

    for (size_t b = 0; b < loadedBranches.size(); ++b)
    {
        if (b > 0)
        {
            pipeline.CreateBranch(loadedBranches[b].Name);
        }
        else
        {
            pipeline.SetBranchName(0, loadedBranches[b].Name);
            pipeline.SetActiveBranchIndex(0);
        }

        for (auto& st : loadedBranches[b].Steps)
        {
            int sIdx = pipeline.AddStep(std::move(st.Operator));
            PipelineStep* addedStep = pipeline.GetStep(sIdx);
            if (addedStep)
            {
                addedStep->SetEnabled(st.Enabled);
            }
        }
    }

    // 反序列化检测任务 (若存在)
    if (taskManager && rootObj.contains("detectionTasks") && rootObj["detectionTasks"].isArray())
    {
        taskManager->FromJson(rootObj["detectionTasks"].toArray());
    }

    pipeline.SetActiveBranchIndex(0);
    if (!pipeline.GetSourceImage().empty())
    {
        pipeline.Execute(true);
    }

    errorMsg.clear();
    Logger::Info("ProjectManager", "工程已成功从 " + filePath.toStdString() + " 导入 (版本: " + version.toStdString() + ")");
    return true;
}

bool ProjectManager::LoadProject(PipelineManager& pipeline, const QString& filePath, QString& errorMsg)
{
    return LoadProject(pipeline, nullptr, filePath, errorMsg);
}
