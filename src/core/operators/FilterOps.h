#pragma once

#include "../IOpNode.h"

/**
 * @brief 高斯滤波算子
 */
class GaussianBlurOp : public IOpNode
{
public:
    GaussianBlurOp();
    ~GaussianBlurOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "GaussianBlur"; }
    std::string GetDisplayName() const override { return "高斯滤波 (Gaussian Blur)"; }
    std::string GetDescription() const override { return "采用二维高斯正态分布核进行平滑降噪，抑制高斯噪声并保留边缘信息。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<GaussianBlurOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 均值滤波算子
 */
class BlurOp : public IOpNode
{
public:
    BlurOp();
    ~BlurOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "Blur"; }
    std::string GetDisplayName() const override { return "均值滤波 (Normalized Blur)"; }
    std::string GetDescription() const override { return "使用归一化矩形卷积核计算邻域像素均值，平滑图像消除随机噪点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BlurOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 中值滤波算子
 */
class MedianBlurOp : public IOpNode
{
public:
    MedianBlurOp();
    ~MedianBlurOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "MedianBlur"; }
    std::string GetDisplayName() const override { return "中值滤波 (Median Blur)"; }
    std::string GetDescription() const override { return "使用邻域像素的中位数替代中心像素，特别擅长去除椒盐噪声且不模糊边缘。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<MedianBlurOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 双边滤波算子 (保边去噪)
 */
class BilateralFilterOp : public IOpNode
{
public:
    BilateralFilterOp();
    ~BilateralFilterOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "BilateralFilter"; }
    std::string GetDisplayName() const override { return "双边滤波 (Bilateral Filter)"; }
    std::string GetDescription() const override { return "结合空间邻近度与像素值相似度的非线性滤波，具有强大的保边平滑去噪能力。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BilateralFilterOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 自定义2D卷积核算子
 */
class Filter2DOp : public IOpNode
{
public:
    Filter2DOp();
    ~Filter2DOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "Filter2D"; }
    std::string GetDisplayName() const override { return "2D卷积自定义核 (Filter2D)"; }
    std::string GetDescription() const override { return "应用自定义 3x3 卷积核进行线性滤波 (内置锐化、浮雕、边缘增强等预设)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<Filter2DOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
