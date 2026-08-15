#pragma once

#include "../IOpNode.h"

/**
 * @brief 亮度与对比度调节算子 (convertScaleAbs)
 */
class BrightnessContrastOp : public IOpNode
{
public:
    BrightnessContrastOp();
    ~BrightnessContrastOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "BrightnessContrast"; }
    std::string GetDisplayName() const override { return "亮度与对比度 (Brightness & Contrast)"; }
    std::string GetDescription() const override { return "线性调节图像增益 (Alpha/对比度) 与偏置 (Beta/亮度)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BrightnessContrastOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 直方图均衡化算子 (equalizeHist)
 */
class EqualizeHistOp : public IOpNode
{
public:
    EqualizeHistOp();
    ~EqualizeHistOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "EqualizeHist"; }
    std::string GetDisplayName() const override { return "直方图均衡化 (Equalize Hist)"; }
    std::string GetDescription() const override { return "拉伸图像灰度动态范围，增强低对比度或背光图像的全局明暗层次。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<EqualizeHistOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 自适应直方图均衡化算子 (CLAHE)
 */
class CLAHEOp : public IOpNode
{
public:
    CLAHEOp();
    ~CLAHEOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "CLAHE"; }
    std::string GetDisplayName() const override { return "限制对比度自适应均衡 (CLAHE)"; }
    std::string GetDescription() const override { return "分块局部均衡化并限制对比度放大倍数，避免全局均衡化过度放大背景噪点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CLAHEOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Gamma 伽马校正算子
 */
class GammaCorrectionOp : public IOpNode
{
public:
    GammaCorrectionOp();
    ~GammaCorrectionOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "GammaCorrection"; }
    std::string GetDisplayName() const override { return "伽马校正 (Gamma Correction)"; }
    std::string GetDescription() const override { return "非线性调节暗部与亮部响应曲线，提亮暗部或压暗过曝区域。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<GammaCorrectionOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像反色算子
 */
class InvertOp : public IOpNode
{
public:
    InvertOp();
    ~InvertOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "Invert"; }
    std::string GetDisplayName() const override { return "图像反色 (Invert / Bitwise Not)"; }
    std::string GetDescription() const override { return "逐像素取反 (255 - pixel)，生成负片效果。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<InvertOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像锐化算子
 */
class SharpenOp : public IOpNode
{
public:
    SharpenOp();
    ~SharpenOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageEnhancement; }
    std::string GetName() const override { return "Sharpen"; }
    std::string GetDisplayName() const override { return "图像锐化 (Sharpen)"; }
    std::string GetDescription() const override { return "通过高频强调增强模糊边缘与微小纹理细节。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<SharpenOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
