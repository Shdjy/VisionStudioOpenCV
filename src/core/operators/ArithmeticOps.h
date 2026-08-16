#pragma once

#include "../IOpNode.h"

/**
 * @brief 双图与原图算术逻辑运算算子 (AddWeighted, AbsDiff, Bitwise AND/OR/XOR, Add, Subtract)
 */
class BlendWithSourceOp : public IOpNode
{
public:
    BlendWithSourceOp();
    ~BlendWithSourceOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "BlendWithSource"; }
    std::string GetDisplayName() const override { return "双图与原图算术复合运算 (Image Arithmetic & Blending)"; }
    std::string GetDescription() const override { return "支持当前图与原始输入图或前置步骤进行加权融合、绝对值差分(缺陷检测)、按位与/或/异或掩码及饱和加减运算。"; }
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
 * @brief 掩码抠图与原图ROI提取算子 (Mask Apply ROI)
 */
class MaskApplyOp : public IOpNode
{
public:
    MaskApplyOp();
    ~MaskApplyOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "MaskApply"; }
    std::string GetDisplayName() const override { return "掩码抠图与原图ROI提取 (Mask Apply ROI)"; }
    std::string GetDescription() const override { return "将当前二值化或分割结果作为掩码(Mask)，作用回原始图像，仅提取目标区域并支持自定义背景填充。"; }
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
