#pragma once

#include "../IOpNode.h"

/**
 * @brief 固定阈值二值化算子
 */
class ThresholdOp : public IOpNode
{
public:
    ThresholdOp();
    ~ThresholdOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Thresholding; }
    std::string GetName() const override { return "Threshold"; }
    std::string GetDisplayName() const override { return "固定阈值分割 (Threshold)"; }
    std::string GetDescription() const override { return "根据设定的固定灰度阈值将图像划分为前景与背景 (二值化/截断/归零)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ThresholdOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Otsu 大津算法自动阈值算子
 */
class OtsuThresholdOp : public IOpNode
{
public:
    OtsuThresholdOp();
    ~OtsuThresholdOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Thresholding; }
    std::string GetName() const override { return "OtsuThreshold"; }
    std::string GetDisplayName() const override { return "大津二值化 (Otsu Threshold)"; }
    std::string GetDescription() const override { return "利用最大类间方差法自动搜索最佳全局二值化分割阈值。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<OtsuThresholdOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 自适应局部阈值算子
 */
class AdaptiveThresholdOp : public IOpNode
{
public:
    AdaptiveThresholdOp();
    ~AdaptiveThresholdOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Thresholding; }
    std::string GetName() const override { return "AdaptiveThreshold"; }
    std::string GetDisplayName() const override { return "自适应局部阈值 (Adaptive)"; }
    std::string GetDescription() const override { return "根据像素邻域块内的灰度统计动态计算每个像素的独立阈值，对抗光照不均。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<AdaptiveThresholdOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 颜色范围提取二值化算子 (inRange)
 */
class InRangeOp : public IOpNode
{
public:
    InRangeOp();
    ~InRangeOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Thresholding; }
    std::string GetName() const override { return "InRange"; }
    std::string GetDisplayName() const override { return "颜色区间提取 (InRange)"; }
    std::string GetDescription() const override { return "根据通道上下限阈值提取目标颜色区间 (常配合 HSV 色彩空间使用)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<InRangeOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
