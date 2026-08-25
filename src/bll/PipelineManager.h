#pragma once

#include "PipelineStep.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <QObject>

/**
 * @brief 单个分支流水线数据结构
 */
struct PipelineBranch
{
    std::string Name;                                  // 分支名称，如 "主流水线 (Main)", "分支 1"
    cv::Mat SourceImage;                               // 该分支的基准输入图像
    std::vector<std::unique_ptr<PipelineStep>> Steps;  // 该分支包含的步骤列表
    double TotalTimeMs = 0.0;                          // 该分支的执行耗时

    PipelineBranch() = default;
    PipelineBranch(PipelineBranch&&) noexcept = default;
    PipelineBranch& operator=(PipelineBranch&&) noexcept = default;
    PipelineBranch(const PipelineBranch&) = delete;
    PipelineBranch& operator=(const PipelineBranch&) = delete;
};

/**
 * @brief 流水线调度管理引擎 (支持多分支管理、单向数据流与增量缓存)
 */
class PipelineManager : public QObject
{
    Q_OBJECT

public:
    explicit PipelineManager(QObject* parent = nullptr);
    ~PipelineManager() override = default;

    // --- 分支生命周期管理 ---
    int GetBranchCount() const { return static_cast<int>(_branches.size()); }
    int GetActiveBranchIndex() const { return _activeBranchIndex; }
    void SetActiveBranchIndex(int index);
    std::string GetBranchName(int index) const;
    void SetBranchName(int index, const std::string& name);

    /**
     * @brief 基于当前活跃分支的最新输出图像创建新分支
     * @param name 分支名称 (为空则自动命名为 "分支 N")
     * @return 新建分支的索引
     */
    int CreateBranch(const std::string& name = "");

    /**
     * @brief 移除指定分支 (主流水线锁定不可删除)
     */
    bool RemoveBranch(int index);

    /**
     * @brief 获取指定分支的基准输入图
     */
    const cv::Mat& GetBranchSourceImage(int branchIndex) const;

    /**
     * @brief 获取指定分支的最终输出图像
     */
    const cv::Mat& GetBranchFinalOutput(int branchIndex) const;
    const cv::Mat& GetBranchFinalOutput(const std::string& branchName) const;

    // --- 当前活跃分支的算子调度 (完全兼容原有接口) ---
    void SetSourceImage(const cv::Mat& img);
    const cv::Mat& GetSourceImage() const;

    int AddStep(std::unique_ptr<IOpNode> opNode);
    bool InsertStep(int index, std::unique_ptr<IOpNode> opNode);
    bool RemoveStep(int index);
    bool MoveStepUp(int index);
    bool MoveStepDown(int index);
    void Clear();

    void SetAllStepsEnabled(bool enabled);
    bool AreAllStepsEnabled() const;
    int GetStepCount() const;

    PipelineStep* GetStep(int index);
    const PipelineStep* GetStep(int index) const;

    void MarkDirtyFrom(int index);
    bool Execute(bool forceAll = false);

    const cv::Mat& GetFinalOutput() const;
    const cv::Mat& GetStepOutput(int index) const;
    double GetTotalTimeMs() const { return _totalTimeMs; }

    // 跨分支获取指定分支的步骤
    int GetBranchStepCount(int branchIndex) const;
    PipelineStep* GetBranchStep(int branchIndex, int stepIndex);
    const PipelineStep* GetBranchStep(int branchIndex, int stepIndex) const;

signals:
    void PipelineChanged();
    void BranchListChanged();
    void ActiveBranchChanged(int index);
    void StepExecuted(int stepIndex, double timeMs);
    void ExecutionCompleted(double totalTimeMs);
    void ExecutionError(int stepIndex, const QString& errorMsg);

private:
    std::vector<PipelineBranch> _branches;
    int _activeBranchIndex = 0;
    cv::Mat _emptyMat;
    double _totalTimeMs = 0.0;

    void EnsureDefaultBranch();
    void InvalidateCrossBranchDependents();
    bool ExecuteBranch(int branchIdx, bool forceAll);
};

