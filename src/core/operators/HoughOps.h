#pragma once

#include "../IOpNode.h"

/**
 * @brief 累计概率霍夫直线检测算子 (HoughLinesP)
 */
class HoughLinesPOp : public IOpNode
{
public:
    HoughLinesPOp();
    ~HoughLinesPOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::HoughTransform; }
    std::string GetName() const override { return "HoughLinesP"; }
    std::string GetDisplayName() const override { return "霍夫直线检测 (HoughLinesP)"; }
    std::string GetDescription() const override { return "采用渐进概率霍夫变换检测二值边缘图像中的直线段并绘制。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<HoughLinesPOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 霍夫圆检测算子 (HoughCircles)
 */
class HoughCirclesOp : public IOpNode
{
public:
    HoughCirclesOp();
    ~HoughCirclesOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::HoughTransform; }
    std::string GetName() const override { return "HoughCircles"; }
    std::string GetDisplayName() const override { return "霍夫圆检测 (HoughCircles)"; }
    std::string GetDescription() const override { return "使用 2-1 霍夫梯度法自动定位图像中的圆形几何特征。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<HoughCirclesOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
