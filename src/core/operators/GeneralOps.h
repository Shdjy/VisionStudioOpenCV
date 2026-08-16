#pragma once

#include "../IOpNode.h"

/**
 * @brief 通道重组算子，使用输入图像现有通道构造新的 BGR 图像
 */
class ChannelMergeOp : public IOpNode
{
public:
    ChannelMergeOp();
    OpCategory GetCategory() const override { return OpCategory::ColorAndChannels; }
    std::string GetName() const override { return "ChannelMerge"; }
    std::string GetDisplayName() const override { return "通道重组 (Merge Channels)"; }
    std::string GetDescription() const override { return "从蓝、绿、红、灰度或常量零通道中选择并合成为新的三通道图像。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ChannelMergeOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 矩阵归一化算子
 */
class NormalizeOp : public IOpNode
{
public:
    NormalizeOp();
    OpCategory GetCategory() const override { return OpCategory::EnhancementAndRestoration; }
    std::string GetName() const override { return "Normalize"; }
    std::string GetDisplayName() const override { return "矩阵归一化 (Normalize)"; }
    std::string GetDescription() const override { return "支持 MINMAX、L1、L2 和无穷范数归一化及输出深度转换。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<NormalizeOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 可分离卷积滤波算子
 */
class SeparableFilterOp : public IOpNode
{
public:
    SeparableFilterOp();
    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "SeparableFilter"; }
    std::string GetDisplayName() const override { return "可分离滤波 (SepFilter2D)"; }
    std::string GetDescription() const override { return "使用两个一维核完成高斯平滑或 Sobel X/Y 可分离卷积。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<SeparableFilterOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 通用二维仿射变换算子
 */
class AffineTransformOp : public IOpNode
{
public:
    AffineTransformOp();
    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "AffineTransform"; }
    std::string GetDisplayName() const override { return "通用仿射变换 (WarpAffine)"; }
    std::string GetDescription() const override { return "组合旋转、缩放、平移和水平/垂直错切并执行 warpAffine。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<AffineTransformOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像像素统计分析算子
 */
class ImageStatisticsOp : public IOpNode
{
public:
    ImageStatisticsOp();
    OpCategory GetCategory() const override { return OpCategory::ColorAndChannels; }
    std::string GetName() const override { return "ImageStatistics"; }
    std::string GetDisplayName() const override { return "像素统计 (Min Max Mean StdDev)"; }
    std::string GetDescription() const override { return "计算各通道最小值、最大值、均值和标准差，并输出结构化结果。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ImageStatisticsOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 当前图与原始图直方图比较算子
 */
class HistogramCompareOp : public IOpNode
{
public:
    HistogramCompareOp();
    OpCategory GetCategory() const override { return OpCategory::FeatureAndMatch; }
    std::string GetName() const override { return "HistogramCompare"; }
    std::string GetDisplayName() const override { return "直方图比较 (CompareHist)"; }
    std::string GetDescription() const override { return "计算当前图像与原始图像灰度直方图的相关性、卡方、交集或巴氏距离。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<HistogramCompareOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 直方图规定化算子
 */
class HistogramMatchOp : public IOpNode
{
public:
    HistogramMatchOp();
    OpCategory GetCategory() const override { return OpCategory::EnhancementAndRestoration; }
    std::string GetName() const override { return "HistogramMatch"; }
    std::string GetDisplayName() const override { return "直方图匹配 (Histogram Specification)"; }
    std::string GetDescription() const override { return "将当前图像各通道的累积分布映射到原始参考图像。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<HistogramMatchOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 最大轮廓图像矩与 Hu 不变矩分析算子
 */
class MomentsHuOp : public IOpNode
{
public:
    MomentsHuOp();
    OpCategory GetCategory() const override { return OpCategory::ContourAnalysis; }
    std::string GetName() const override { return "MomentsHu"; }
    std::string GetDisplayName() const override { return "图像矩与 Hu 矩 (Moments)"; }
    std::string GetDescription() const override { return "提取最大轮廓，计算面积、质心和七个对平移缩放旋转稳定的 Hu 不变矩。"; }
    std::string GetPrincipleDoc() const override;
    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }
    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<MomentsHuOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
