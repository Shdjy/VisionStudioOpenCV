#pragma once

#include "../IOpNode.h"

/**
 * @brief 图像几何缩放算子 (resize)
 */
class ResizeOp : public IOpNode
{
public:
    ResizeOp();
    ~ResizeOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "Resize"; }
    std::string GetDisplayName() const override { return "几何缩放 (Resize)"; }
    std::string GetDescription() const override { return "按比例因子或固定分辨率调整图像尺寸，支持双线性、双三次与区域插值算法。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ResizeOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像任意角度旋转算子 (warpAffine)
 */
class RotateOp : public IOpNode
{
public:
    RotateOp();
    ~RotateOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "Rotate"; }
    std::string GetDisplayName() const override { return "几何旋转 (Rotate)"; }
    std::string GetDescription() const override { return "以图像中心为基准点，按指定角度与缩放系数进行仿射旋转变换。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<RotateOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像镜像翻转算子 (flip)
 */
class FlipOp : public IOpNode
{
public:
    FlipOp();
    ~FlipOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "Flip"; }
    std::string GetDisplayName() const override { return "镜像翻转 (Flip)"; }
    std::string GetDescription() const override { return "沿水平轴、垂直轴或对角双向对图像进行镜像变换。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<FlipOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 区域裁剪算子 (Crop / ROI)
 */
class CropRoiOp : public IOpNode
{
public:
    CropRoiOp();
    ~CropRoiOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::GeometricTransform; }
    std::string GetName() const override { return "CropROI"; }
    std::string GetDisplayName() const override { return "ROI 区域裁剪 (Crop ROI)"; }
    std::string GetDescription() const override { return "按指定的相对百分比坐标提取图像的目标感兴趣区域 (ROI)。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CropRoiOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
