#include "DetectionTaskManager.h"
#include "ObjectCounter.h"
#include "DefectDetector.h"
#include "../utils/Logger.h"

DetectionTaskManager::DetectionTaskManager(QObject* parent)
    : QObject(parent)
{
    // 初始化默认示范任务
    AddTask(CreateDefaultCountTask(12));
    AddTask(CreateDefaultDefectTask());
}

void DetectionTaskManager::SetActiveTaskIndex(int index)
{
    if (index >= 0 && index < static_cast<int>(_tasks.size()) && index != _activeTaskIndex)
    {
        _activeTaskIndex = index;
        emit ActiveTaskChanged(index);
    }
}

DetectionTask* DetectionTaskManager::GetTask(int index)
{
    if (index >= 0 && index < static_cast<int>(_tasks.size()))
    {
        return &_tasks[index];
    }
    return nullptr;
}

const DetectionTask* DetectionTaskManager::GetTask(int index) const
{
    if (index >= 0 && index < static_cast<int>(_tasks.size()))
    {
        return &_tasks[index];
    }
    return nullptr;
}

DetectionTask* DetectionTaskManager::GetActiveTask()
{
    return GetTask(_activeTaskIndex);
}

const DetectionTask* DetectionTaskManager::GetActiveTask() const
{
    return GetTask(_activeTaskIndex);
}

int DetectionTaskManager::AddTask(const DetectionTask& task)
{
    _tasks.push_back(task);
    int newIndex = static_cast<int>(_tasks.size()) - 1;
    Logger::Info("DetectionTaskManager", "新增检测任务: " + task.Name);
    emit TaskListChanged();
    return newIndex;
}

bool DetectionTaskManager::RemoveTask(int index)
{
    if (index >= 0 && index < static_cast<int>(_tasks.size()))
    {
        _tasks.erase(_tasks.begin() + index);
        if (_activeTaskIndex >= static_cast<int>(_tasks.size()))
        {
            _activeTaskIndex = std::max(0, static_cast<int>(_tasks.size()) - 1);
        }
        emit TaskListChanged();
        emit ActiveTaskChanged(_activeTaskIndex);
        return true;
    }
    return false;
}

void DetectionTaskManager::Clear()
{
    _tasks.clear();
    _activeTaskIndex = 0;
    emit TaskListChanged();
    emit ActiveTaskChanged(0);
}

DetectionTask DetectionTaskManager::CreateDefaultCountTask(int expectedCount)
{
    DetectionTask task;
    task.Id = "task_bottle_cap_counting";
    task.Name = "瓶盖/目标数量统计任务";
    task.Type = DetectionTaskType::ObjectCounting;
    task.Enabled = true;

    ObjectCountConfig cfg;
    cfg.Method = "ConnectedComponents";
    cfg.ThresholdMethod = "Otsu";
    cfg.InvertThreshold = false;
    cfg.MinArea = 50.0;
    cfg.MaxArea = 500000.0;
    cfg.TargetClass = "bottle_cap";
    cfg.TargetDisplayName = "瓶盖";
    task.Configuration = cfg.ToJson();

    // 默认规则：期望等于 12 个，否则判定 NG
    QJsonObject countRule;
    countRule["name"] = "瓶盖满数检验 (12个)";
    countRule["ruleType"] = "count";
    countRule["operator"] = "==";
    countRule["expectedCount"] = expectedCount;

    QJsonArray rules;
    rules.append(countRule);
    task.Rules = rules;

    return task;
}

DetectionTask DetectionTaskManager::CreateDefaultDefectTask()
{
    DetectionTask task;
    task.Id = "task_metal_defect";
    task.Name = "金属表面划痕与凹坑缺陷检测";
    task.Type = DetectionTaskType::DefectDetection;
    task.Enabled = true;

    DefectDetectConfig cfg;
    cfg.Method = "BackgroundSubtraction";
    cfg.BackgroundBlurKernel = 31;
    cfg.DiffThreshold = 25;
    cfg.DetectBrightDefects = true;
    cfg.DetectDarkDefects = true;
    cfg.MinDefectArea = 15.0;
    cfg.MaxDefectArea = 50000.0;
    cfg.MinDefectLength = 4.0;
    task.Configuration = cfg.ToJson();

    // 默认判定规则：零划痕，凹坑面积小于 80 像素
    QJsonArray rules;

    QJsonObject scratchRule;
    scratchRule["name"] = "零划痕限制规则";
    scratchRule["ruleType"] = "defect";
    scratchRule["defectClass"] = "scratch";
    scratchRule["maxCount"] = 0;
    rules.append(scratchRule);

    QJsonObject pitRule;
    pitRule["name"] = "凹坑面积上限规则";
    pitRule["ruleType"] = "defect";
    pitRule["defectClass"] = "pit";
    pitRule["maxCount"] = 3;
    pitRule["maxArea"] = 80.0;
    rules.append(pitRule);

    task.Rules = rules;

    return task;
}

QJsonArray DetectionTaskManager::ToJson() const
{
    QJsonArray arr;
    for (const auto& task : _tasks)
    {
        arr.append(task.ToJson());
    }
    return arr;
}

bool DetectionTaskManager::FromJson(const QJsonArray& arr)
{
    _tasks.clear();
    for (int i = 0; i < arr.size(); ++i)
    {
        QJsonObject obj = arr[i].toObject();
        _tasks.push_back(DetectionTask::FromJson(obj));
    }
    _activeTaskIndex = 0;
    emit TaskListChanged();
    emit ActiveTaskChanged(0);
    return true;
}
