#pragma once

#include "../IOpNode.h"

// ============================================================================
// 1. LUT 与伪彩色查找表算子
// ============================================================================
class LUTOp : public IOpNode
{
public:
    LUTOp();
    ~LUTOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ColorAndChannels; }
    std::string GetName() const override { return "LUT"; }
    std::string GetDisplayName() const override { return "LUT查找表与伪彩色 (LUT & Colormap)"; }
    std::string GetDescription() const override { return "利用预定义查找表或 22 种科学伪彩色 (Jet/Viridis/Hot等) 将灰度映射为绚丽热力图。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<LUTOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 2. 透视变换与梯形畸变校正
// ============================================================================
class PerspectiveTransformOp : public IOpNode
{
public:
    PerspectiveTransformOp();
    ~PerspectiveTransformOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "PerspectiveTransform"; }
    std::string GetDisplayName() const override { return "透视变换与梯形校正 (Perspective Warp)"; }
    std::string GetDescription() const override { return "通过 3x3 透视单应性矩阵实现倾斜视角校正、鸟瞰图投影及四边形拉伸。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<PerspectiveTransformOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 3. 极坐标变换与圆环展开
// ============================================================================
class PolarTransformOp : public IOpNode
{
public:
    PolarTransformOp();
    ~PolarTransformOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "PolarTransform"; }
    std::string GetDisplayName() const override { return "极坐标变换与圆环展开 (Polar Transform)"; }
    std::string GetDescription() const override { return "将圆形或圆环工件表面展开为长条矩形，用于瓶盖、轴承圆周字符与缺陷检测。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<PolarTransformOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 4. 图像金字塔 (pyrDown / pyrUp)
// ============================================================================
class ImagePyramidOp : public IOpNode
{
public:
    ImagePyramidOp();
    ~ImagePyramidOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "ImagePyramid"; }
    std::string GetDisplayName() const override { return "图像金字塔采样 (Image Pyramid)"; }
    std::string GetDescription() const override { return "通过高斯金字塔 pyrDown 进行 1/2 降采样平滑或 pyrUp 进行 2x 上采样。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ImagePyramidOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 5. 图像连通域分析与多色标记 (connectedComponentsWithStats)
// ============================================================================
class ConnectedComponentsOp : public IOpNode
{
public:
    ConnectedComponentsOp();
    ~ConnectedComponentsOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ContourAnalysis; }
    std::string GetName() const override { return "ConnectedComponents"; }
    std::string GetDisplayName() const override { return "连通域分析与多色标记 (Connected Components)"; }
    std::string GetDescription() const override { return "分析二值图像连通区域并统计各连通块面积、外接矩形与质心，赋予随机伪彩独立区分。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ConnectedComponentsOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 6. 图像距离变换 (distanceTransform)
// ============================================================================
class DistanceTransformOp : public IOpNode
{
public:
    DistanceTransformOp();
    ~DistanceTransformOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ContourAnalysis; }
    std::string GetName() const override { return "DistanceTransform"; }
    std::string GetDisplayName() const override { return "距离变换 (Distance Transform)"; }
    std::string GetDescription() const override { return "计算二值图像中前景像素到最近零像素的几何距离，用于骨架提取、中心定位与分水岭预处理。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<DistanceTransformOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 7. 几何直线拟合 (fitLine)
// ============================================================================
class FitLineOp : public IOpNode
{
public:
    FitLineOp();
    ~FitLineOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::HoughTransform; }
    std::string GetName() const override { return "FitLine"; }
    std::string GetDisplayName() const override { return "亚像素直线拟合 (Fit Line)"; }
    std::string GetDescription() const override { return "使用 M-估计或最小二乘法对边缘点集进行稳健直线拟合，输出高精度直线方程与斜率。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<FitLineOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 8. 图像缺陷修复与去水印 (inpaint)
// ============================================================================
class InpaintOp : public IOpNode
{
public:
    InpaintOp();
    ~InpaintOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EnhancementAndRestoration; }
    std::string GetName() const override { return "Inpaint"; }
    std::string GetDisplayName() const override { return "图像瑕疵修复与去划痕 (Inpaint)"; }
    std::string GetDescription() const override { return "基于 Telea 或 Navier-Stokes 偏微分方程对划痕、噪点或遮挡区域进行智能无痕修补。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<InpaintOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 9. 方框积分图滤波 (boxFilter)
// ============================================================================
class BoxFilterOp : public IOpNode
{
public:
    BoxFilterOp();
    ~BoxFilterOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "BoxFilter"; }
    std::string GetDisplayName() const override { return "方框盒式滤波 (Box Filter)"; }
    std::string GetDescription() const override { return "使用均一全 1 核进行快速盒式积分图滤波，支持归一化与非归一化模式。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BoxFilterOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 10. 工业仿真加噪测试 (Noise Injection)
// ============================================================================
class NoiseInjectionOp : public IOpNode
{
public:
    NoiseInjectionOp();
    ~NoiseInjectionOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EnhancementAndRestoration; }
    std::string GetName() const override { return "NoiseInjection"; }
    std::string GetDisplayName() const override { return "工业仿真加噪测试 (Noise Injection)"; }
    std::string GetDescription() const override { return "向图像注入椒盐噪声或高斯随机噪声，用于检验滤波算法与流水线的抗噪鲁棒性。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<NoiseInjectionOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
