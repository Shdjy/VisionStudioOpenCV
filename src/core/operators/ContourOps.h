#pragma once

#include "../IOpNode.h"

/**
 * @brief 轮廓查找、多维度特征过滤与几何分析绘制算子
 */
class FindContoursOp : public IOpNode
{
public:
    FindContoursOp();
    ~FindContoursOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::ContourAnalysis; }
    std::string GetName() const override { return "FindContours"; }
    std::string GetDisplayName() const override { return "轮廓查找与特征过滤 (Find & Filter Contours)"; }
    std::string GetDescription() const override { return "提取图像轮廓，支持按最大/最小面积、宽度、高度、长宽比、圆度及Top-K多维度自定义筛选与标注。"; }
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
