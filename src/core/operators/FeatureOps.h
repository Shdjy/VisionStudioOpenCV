#pragma once

#include "../IOpNode.h"

/**
 * @brief Harris 角点检测算子 (cornerHarris)
 */
class CornerHarrisOp : public IOpNode
{
public:
    CornerHarrisOp();
    ~CornerHarrisOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::FeatureAndMatch; }
    std::string GetName() const override { return "CornerHarris"; }
    std::string GetDisplayName() const override { return "Harris 角点检测 (CornerHarris)"; }
    std::string GetDescription() const override { return "利用自相关矩阵的二阶导数特征值检测图像中的显著角点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CornerHarrisOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Shi-Tomasi 角点检测算子 (goodFeaturesToTrack)
 */
class GoodFeaturesToTrackOp : public IOpNode
{
public:
    GoodFeaturesToTrackOp();
    ~GoodFeaturesToTrackOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::FeatureAndMatch; }
    std::string GetName() const override { return "GoodFeaturesToTrack"; }
    std::string GetDisplayName() const override { return "Shi-Tomasi 角点检测 (GoodFeatures)"; }
    std::string GetDescription() const override { return "计算最小特征值 $\\min(\\lambda_1, \\lambda_2)$，提取高质量且分布均匀的特征角点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<GoodFeaturesToTrackOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 模板匹配与目标定位算子 (matchTemplate)
 */
class MatchTemplateOp : public IOpNode
{
public:
    MatchTemplateOp();
    ~MatchTemplateOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::FeatureAndMatch; }
    std::string GetName() const override { return "MatchTemplate"; }
    std::string GetDisplayName() const override { return "模板匹配与定位 (Match Template)"; }
    std::string GetDescription() const override { return "在图像中通过滑窗搜索指定 ROI 模板，支持单目标/多目标匹配、阈值过滤与亚像素高亮标定。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<MatchTemplateOp>(*this); }

    void SetTemplateRoi(int x, int y, int w, int h);

private:
    std::vector<OpParameter> _params;
};
