#pragma once

#include "DetectionTask.h"
#include <vector>
#include <memory>
#include <QObject>

/**
 * @brief 工业检测任务管理中心 (DetectionTaskManager)
 */
class DetectionTaskManager : public QObject
{
    Q_OBJECT

public:
    explicit DetectionTaskManager(QObject* parent = nullptr);
    ~DetectionTaskManager() override = default;

    int GetTaskCount() const { return static_cast<int>(_tasks.size()); }
    int GetActiveTaskIndex() const { return _activeTaskIndex; }
    void SetActiveTaskIndex(int index);

    DetectionTask* GetTask(int index);
    const DetectionTask* GetTask(int index) const;

    DetectionTask* GetActiveTask();
    const DetectionTask* GetActiveTask() const;

    int AddTask(const DetectionTask& task);
    bool RemoveTask(int index);
    void Clear();

    /**
     * @brief 创建标准目标计数任务模板
     */
    static DetectionTask CreateDefaultCountTask(int expectedCount = 12);

    /**
     * @brief 创建标准表面缺陷检测任务模板
     */
    static DetectionTask CreateDefaultDefectTask();

    /**
     * @brief 序列化所有任务到 JSON 数组
     */
    QJsonArray ToJson() const;

    /**
     * @brief 从 JSON 数组反序列化加载任务
     */
    bool FromJson(const QJsonArray& arr);

signals:
    void TaskListChanged();
    void ActiveTaskChanged(int index);

private:
    std::vector<DetectionTask> _tasks;
    int _activeTaskIndex = 0;
};
