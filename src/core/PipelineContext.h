#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <mutex>

/**
 * @brief 流水线全局多图上下文 (为双图运算、原图融合与掩码抠图提供数据源)
 */
class PipelineContext
{
public:
    static PipelineContext& Instance()
    {
        static PipelineContext ctx;
        return ctx;
    }

    void SetSourceImage(const cv::Mat& src)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sourceImage = src.clone();
    }

    cv::Mat GetSourceImage() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sourceImage.empty() ? cv::Mat() : _sourceImage.clone();
    }

    void SetStepOutput(int stepIdx, const cv::Mat& outMat)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (stepIdx >= static_cast<int>(_stepOutputs.size()))
        {
            _stepOutputs.resize(stepIdx + 1);
        }
        _stepOutputs[stepIdx] = outMat.clone();
    }

    cv::Mat GetStepOutput(int stepIdx) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (stepIdx >= 0 && stepIdx < static_cast<int>(_stepOutputs.size()) && !_stepOutputs[stepIdx].empty())
        {
            return _stepOutputs[stepIdx].clone();
        }
        return _sourceImage.empty() ? cv::Mat() : _sourceImage.clone();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stepOutputs.clear();
    }

private:
    PipelineContext() = default;
    mutable std::mutex _mutex;
    cv::Mat _sourceImage;
    std::vector<cv::Mat> _stepOutputs;
};
