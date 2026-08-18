#pragma once

#include "../IOpNode.h"

enum class MaskShape
{
    Rectangle = 0,       // 矩形选区
    Circle = 1,          // 圆形/椭圆选区
    Ring = 2,            // 圆环/椭圆环选区
    RegularPolygon = 3,  // 正多边形选区 (3~12边形)
    RotatedRect = 4,     // 旋转矩形选区
    ThresholdAuto = 5    // 原图亮度阈值自动掩码
};

enum class MaskAction
{
    KeepInsideRoi = 0,       // 仅保留掩码内区域 (背景置黑)
    KeepInsideRoiWhiteBg = 1,// 仅保留掩码内区域 (背景置白)
    ExcludeInsideRoi = 2,    // 剔除掩码内区域
    LocalBlur = 3,           // 局部高斯滤波
    LocalSharpen = 4,        // 局部USM锐化
    LocalEnhance = 5,        // 局部对比度增强
    HighlightOverlay = 6,    // 半透明红色高亮标注
    OutputMaskOnly = 7       // 仅输出二值掩码
};

/**
 * @brief 交互式几何与阈值掩码局部复合算子
 */
class InteractiveMaskOp : public IOpNode
{
public:
    InteractiveMaskOp();
    ~InteractiveMaskOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "InteractiveMask"; }
    std::string GetDisplayName() const override { return "交互式掩码与局部ROI处理 (Interactive Mask ROI)"; }
    std::string GetDescription() const override { return "通过鼠标拖动绘制矩形/圆形/椭圆形状或阈值自动抠图，支持局部滤波、锐化、抠图提取与半透明高亮叠加。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<InteractiveMaskOp>(*this); }

    // 辅助接口
    void SetRoiGeometry(int x, int y, int w, int h);
    void SetShape(MaskShape shape);
    void SetAction(MaskAction action);

private:
    std::vector<OpParameter> _params;
};
