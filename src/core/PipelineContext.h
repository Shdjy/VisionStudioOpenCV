#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>
#include <mutex>

/**
 * @brief 流水线全局多图与分支上下文 (为双图运算、分支流程、原图融合与掩码抠图提供数据源)
 */
class PipelineContext
{
public:
    static PipelineContext& Instance()
    {
        static PipelineContext ctx;
        return ctx;
    }

    /**
     * @brief 设置原始输入图像
     */
    void SetSourceImage(const cv::Mat& src)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sourceImage = src.clone();
    }

    /**
     * @brief 获取原始输入图像
     */
    cv::Mat GetSourceImage() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sourceImage.empty() ? cv::Mat() : _sourceImage.clone();
    }

    /**
     * @brief 记录步骤输出缓存
     */
    void SetStepOutput(int stepIdx, const cv::Mat& outMat)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (stepIdx >= static_cast<int>(_stepOutputs.size()))
        {
            _stepOutputs.resize(stepIdx + 1);
        }
        _stepOutputs[stepIdx] = outMat.clone();
    }

    /**
     * @brief 获取指定步骤的输出图像
     */
    cv::Mat GetStepOutput(int stepIdx) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (stepIdx >= 0 && stepIdx < static_cast<int>(_stepOutputs.size()) && !_stepOutputs[stepIdx].empty())
        {
            return _stepOutputs[stepIdx].clone();
        }
        return _sourceImage.empty() ? cv::Mat() : _sourceImage.clone();
    }

    /**
     * @brief 暂存图像到命名分支槽位
     * @param name 分支名称或标识 (如 "Branch 1", "Branch 2")
     * @param branchMat 暂存的图像数据
     */
    void SetBranch(const std::string& name, const cv::Mat& branchMat)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _branchOutputs[name] = branchMat.clone();
    }

    /**
     * @brief 获取命名分支槽位的图像
     * @param name 分支名称或标识
     * @return 分支图像 (若不存在则返回原图)
     */
    cv::Mat GetBranch(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _branchOutputs.find(name);
        if (it != _branchOutputs.end() && !it->second.empty())
        {
            return it->second.clone();
        }
        return _sourceImage.empty() ? cv::Mat() : _sourceImage.clone();
    }

    /**
     * @brief 判断是否存在该分支输出
     */
    bool HasBranch(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _branchOutputs.find(name);
        return it != _branchOutputs.end() && !it->second.empty();
    }

    /**
     * @brief 获取当前所有已暂存分支的标识列表
     */
    std::vector<std::string> GetAvailableBranchNames() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<std::string> names;
        names.reserve(_branchOutputs.size());
        for (const auto& pair : _branchOutputs)
        {
            if (!pair.second.empty())
            {
                names.push_back(pair.first);
            }
        }
        return names;
    }

    /**
     * @brief 清空所有步骤与分支缓存
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stepOutputs.clear();
        _branchOutputs.clear();
    }

private:
    PipelineContext() = default;
    mutable std::mutex _mutex;
    cv::Mat _sourceImage;
    std::vector<cv::Mat> _stepOutputs;
    std::map<std::string, cv::Mat> _branchOutputs;
};
