#pragma once

#include "../IOpNode.h"

/**
 * @brief 轮廓查找与分析绘制算子 (findContours & drawContours)
 */
class FindContoursOp : public IOpNode
{
public:
    FindContoursOp();
    ~FindContoursOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ContourAnalysis; }
    std::string GetName() const override { return "FindContours"; }
    std::string GetDisplayName() const override { return "轮廓查找与分析 (Find Contours)"; }
    std::string GetDescription() const override { return "提取二值图像中的连通域轮廓，支持面积过滤、外接矩形、最小外接圆与凸包绘制。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<FindContoursOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
