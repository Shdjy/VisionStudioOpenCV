#pragma once

#include "PipelineStep.h"
#include <vector>
#include <memory>
#include <functional>
#include <QObject>

/**
 * @brief 流水线调度管理引擎 (单向数据流与增量缓存)
 */
class PipelineManager : public QObject
{
    Q_OBJECT

public:
    explicit PipelineManager(QObject* parent = nullptr);
    ~PipelineManager() override = default;

    /**
     * @brief 设置输入源图像
     */
    void SetSourceImage(const cv::Mat& img);
    const cv::Mat& GetSourceImage() const { return _sourceImage; }

    /**
     * @brief 添加算子到流水线尾部
     */
    int AddStep(std::unique_ptr<IOpNode> opNode);

    /**
     * @brief 插入算子到指定位置
     */
    bool InsertStep(int index, std::unique_ptr<IOpNode> opNode);

    /**
     * @brief 移除指定步骤
     */
    bool RemoveStep(int index);

    /**
     * @brief 上移步骤
     */
    bool MoveStepUp(int index);

    /**
     * @brief 下移步骤
     */
    bool MoveStepDown(int index);

    /**
     * @brief 清空流水线
     */
    void Clear();

    /**
     * @brief 步骤数量
     */
    int GetStepCount() const { return static_cast<int>(_steps.size()); }

    /**
     * @brief 获取指定步骤指针
     */
    PipelineStep* GetStep(int index);
    const PipelineStep* GetStep(int index) const;

    /**
     * @brief 标记某步及之后的所有步骤为脏状态
     */
    void MarkDirtyFrom(int index);

    /**
     * @brief 执行流水线计算 (增量缓存加速)
     * @param forceAll 是否强制忽略缓存全部重算
     * @return 是否计算成功
     */
    bool Execute(bool forceAll = false);

    /**
     * @brief 获取最终流水线输出图像
     */
    const cv::Mat& GetFinalOutput() const;

    /**
     * @brief 获取某一步的输出结果
     */
    const cv::Mat& GetStepOutput(int index) const;

    /**
     * @brief 获取总耗时 (毫秒)
     */
    double GetTotalTimeMs() const { return _totalTimeMs; }

signals:
    void PipelineChanged();
    void StepExecuted(int stepIndex, double timeMs);
    void ExecutionCompleted(double totalTimeMs);
    void ExecutionError(int stepIndex, const QString& errorMsg);

private:
    cv::Mat _sourceImage;
    cv::Mat _emptyMat;
    std::vector<std::unique_ptr<PipelineStep>> _steps;
    double _totalTimeMs = 0.0;
};
