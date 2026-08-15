#pragma once

#include "../IOpNode.h"

/**
 * @brief 图像膨胀算子
 */
class DilateOp : public IOpNode
{
public:
    DilateOp();
    ~DilateOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Morphology; }
    std::string GetName() const override { return "Dilate"; }
    std::string GetDisplayName() const override { return "膨胀运算 (Dilate)"; }
    std::string GetDescription() const override { return "使用结构元素扩大图像中的高亮（前景）区域，填补前景中的细小孔洞。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<DilateOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 图像腐蚀算子
 */
class ErodeOp : public IOpNode
{
public:
    ErodeOp();
    ~ErodeOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Morphology; }
    std::string GetName() const override { return "Erode"; }
    std::string GetDisplayName() const override { return "腐蚀运算 (Erode)"; }
    std::string GetDescription() const override { return "使用结构元素收缩图像中的高亮（前景）区域，消除孤立细小的白色噪点。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ErodeOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 高级形态学运算 (morphologyEx)
 */
class MorphologyExOp : public IOpNode
{
public:
    MorphologyExOp();
    ~MorphologyExOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::Morphology; }
    std::string GetName() const override { return "MorphologyEx"; }
    std::string GetDisplayName() const override { return "高级形态学 (MorphologyEx)"; }
    std::string GetDescription() const override { return "执行开运算、闭运算、形态学梯度、顶帽与黑帽等复合形态学滤波。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<MorphologyExOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
