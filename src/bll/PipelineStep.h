#pragma once

#include "../core/IOpNode.h"
#include <memory>
#include <string>

/**
 * @brief 流水线单个步骤封装类
 */
class PipelineStep
{
public:
    explicit PipelineStep(std::unique_ptr<IOpNode> opNode);
    ~PipelineStep() = default;

    // 禁用拷贝，允许移动
    PipelineStep(const PipelineStep&) = delete;
    PipelineStep& operator=(const PipelineStep&) = delete;
    PipelineStep(PipelineStep&&) noexcept = default;
    PipelineStep& operator=(PipelineStep&&) noexcept = default;

    /**
     * @brief 获取算子实例指针
     */
    IOpNode* GetOpNode() const { return _opNode.get(); }

    /**
     * @brief 是否启用此步骤
     */
    bool IsEnabled() const { return _enabled; }
    void SetEnabled(bool enabled)
    {
        if (_enabled != enabled)
        {
            _enabled = enabled;
            _isDirty = true;
        }
    }

    /**
     * @brief 脏标记状态
     */
    bool IsDirty() const { return _isDirty; }
    void MarkDirty() { _isDirty = true; }
    void ClearDirty() { _isDirty = false; }

    /**
     * @brief 缓存输出图像
     */
    const cv::Mat& GetOutputCache() const { return _outputCache; }
    void SetOutputCache(const cv::Mat& mat) { _outputCache = mat; }

    /**
     * @brief 本步耗时 (毫秒)
     */
    double GetExecutionTimeMs() const { return _executionTimeMs; }
    void SetExecutionTimeMs(double ms) { _executionTimeMs = ms; }

    /**
     * @brief 错误信息
     */
    const std::string& GetErrorMessage() const { return _errorMessage; }
    void SetErrorMessage(const std::string& err) { _errorMessage = err; }
    bool HasError() const { return !_errorMessage.empty(); }

private:
    std::unique_ptr<IOpNode> _opNode;
    bool _enabled = true;
    bool _isDirty = true;
    cv::Mat _outputCache;
    double _executionTimeMs = 0.0;
    std::string _errorMessage;
};
