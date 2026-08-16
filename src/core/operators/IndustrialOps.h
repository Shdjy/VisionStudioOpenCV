#pragma once

#include "../IOpNode.h"

// ============================================================================
// 1. 圆柱面曲面展开与重映射算子 (Cylindrical Unrolling)
// ============================================================================
class CylindricalUnrollOp : public IOpNode
{
public:
    CylindricalUnrollOp();
    ~CylindricalUnrollOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "CylindricalUnroll"; }
    std::string GetDisplayName() const override { return "🛢️ 圆柱面展开 (Cylindrical Unrolling)"; }
    std::string GetDescription() const override { return "基于圆柱面投影模型与几何重映射(Remap)，将曲面瓶身/易拉罐上的标签展开为平整平面。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CylindricalUnrollOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 2. RANSAC 鲁棒圆/椭圆拟合算子 (RANSAC Circle & Ellipse Fitting)
// ============================================================================
class RansacFitCircleOp : public IOpNode
{
public:
    RansacFitCircleOp();
    ~RansacFitCircleOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::HoughTransform; }
    std::string GetName() const override { return "RansacFitCircle"; }
    std::string GetDisplayName() const override { return "🎯 RANSAC鲁棒圆拟合 (RANSAC Circle Fit)"; }
    std::string GetDescription() const override { return "基于随机抽样一致性(RANSAC)从边缘轮廓中高精度拟合圆与椭圆，强力剔除反光与缺口杂点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<RansacFitCircleOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 3. 工业条码与二维码检测识别算子 (Barcode & QR Code Detection)
// ============================================================================
class BarcodeQrOp : public IOpNode
{
public:
    BarcodeQrOp();
    ~BarcodeQrOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::FeatureAndMatch; }
    std::string GetName() const override { return "BarcodeQrDetector"; }
    std::string GetDisplayName() const override { return "🏷️ 条码与二维码识别 (Barcode & QR)"; }
    std::string GetDescription() const override { return "自动定位并解码工业常见 QR 码与一维条码，在视口中绘制定位边界框与解码文本。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BarcodeQrOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

// ============================================================================
// 4. 骨架提取与图像细化算子 (Skeletonization & Thinning)
// ============================================================================
class SkeletonThinningOp : public IOpNode
{
public:
    SkeletonThinningOp();
    ~SkeletonThinningOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Morphology; }
    std::string GetName() const override { return "SkeletonThinning"; }
    std::string GetDisplayName() const override { return "🦴 骨架提取与细化 (Skeleton Thinning)"; }
    std::string GetDescription() const override { return "对二值化图像执行 Zhang-Suen 细化或形态学骨架迭代，提取线状物体单像素中心骨架。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<SkeletonThinningOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
