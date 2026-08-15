#pragma once

#include "../IOpNode.h"

/**
 * @brief 色彩空间转换算子
 */
class CvtColorOp : public IOpNode
{
public:
    CvtColorOp();
    ~CvtColorOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ColorConversion; }
    std::string GetName() const override { return "CvtColor"; }
    std::string GetDisplayName() const override { return "色彩空间转换 (CvtColor)"; }
    std::string GetDescription() const override { return "在 BGR、灰度 (GRAY)、HSV、Lab、YCrCb 等色彩空间之间相互转换。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CvtColorOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 单通道提取/色彩分离算子
 */
class ChannelSplitOp : public IOpNode
{
public:
    ChannelSplitOp();
    ~ChannelSplitOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ColorConversion; }
    std::string GetName() const override { return "ChannelSplit"; }
    std::string GetDisplayName() const override { return "通道分离提取 (Channel Split)"; }
    std::string GetDescription() const override { return "提取多通道图像中的某一个单独颜色通道 (如 B/G/R 或 H/S/V)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ChannelSplitOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
