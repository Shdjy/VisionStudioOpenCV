#pragma once

#include "../IOpNode.h"

/**
 * @brief 双图与分支算术复合运算算子 (加权融合, 绝对值差分, 按位逻辑, 代数加减乘除)
 */
class BlendWithSourceOp : public IOpNode
{
public:
    BlendWithSourceOp();
    ~BlendWithSourceOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "BlendWithSource"; }
    std::string GetDisplayName() const override { return "双图与分支复合运算 (Branch Merge & Blend)"; }
    std::string GetDescription() const override { return "将当前流程的图像与指定分支（主流水线/分支1~4/原图/纯色底板）的结果进行加权融合、差分缺陷检测、按位逻辑或代数运算。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BlendWithSourceOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 掩码抠图与ROI提取算子 (Mask Apply ROI)
 */
class MaskApplyOp : public IOpNode
{
public:
    MaskApplyOp();
    ~MaskApplyOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "MaskApply"; }
    std::string GetDisplayName() const override { return "掩码抠图与ROI提取 (Mask Apply ROI)"; }
    std::string GetDescription() const override { return "将当前二值分割掩码作用到目标图像（原图/主流水线/指定分支），高保真提取目标区域并支持背景填充。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<MaskApplyOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 阈值截断与原图截取算子 (Threshold Truncation & ToZero)
 */
class ThresholdTruncateOp : public IOpNode
{
public:
    ThresholdTruncateOp();
    ~ThresholdTruncateOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Thresholding; }
    std::string GetName() const override { return "ThresholdTruncate"; }
    std::string GetDisplayName() const override { return "阈值截断与区间截取 (Threshold Truncate)"; }
    std::string GetDescription() const override { return "提供 THRESH_TRUNC (大于阈值截平)、THRESH_TOZERO (低于阈值置零) 等非二值化像素级截断。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ThresholdTruncateOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
