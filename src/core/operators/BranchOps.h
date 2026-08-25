#pragma once

#include "../IOpNode.h"

/**
 * @brief 分支暂存与锚点标记算子 (Branch Save / Fork)
 */
class BranchSaveOp : public IOpNode
{
public:
    BranchSaveOp();
    ~BranchSaveOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "BranchSave"; }
    std::string GetDisplayName() const override { return "分支暂存与锚点标记 (Branch Save / Fork)"; }
    std::string GetDescription() const override { return "将当前步骤的处理结果暂存到指定分支槽位 (Branch 1~4)，便于后续分支切换或双图运算调用，同时将图像直通传递给下一步。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BranchSaveOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief 分支加载与源切换算子 (Branch Load / Switch Source)
 */
class BranchLoadOp : public IOpNode
{
public:
    BranchLoadOp();
    ~BranchLoadOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::MaskAndArithmetic; }
    std::string GetName() const override { return "BranchLoad"; }
    std::string GetDisplayName() const override { return "分支加载与源切换 (Branch Load / Switch Source)"; }
    std::string GetDescription() const override { return "将后续流程的输入图像重定向为指定分支、前置步骤输出或原始输入图像，从而开启全新的分支处理流水线。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<BranchLoadOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
