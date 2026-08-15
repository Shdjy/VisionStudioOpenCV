#pragma once

#include "../IOpNode.h"

/**
 * @brief Canny 边缘检测算子
 */
class CannyOp : public IOpNode
{
public:
    CannyOp();
    ~CannyOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EdgeDetection; }
    std::string GetName() const override { return "Canny"; }
    std::string GetDisplayName() const override { return "Canny 边缘检测 (Canny)"; }
    std::string GetDescription() const override { return "经典多阶段边缘检测算法，包含高斯滤波、梯度计算、非极大值抑制与双阈值滞后连接。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<CannyOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Sobel 一阶微分边缘梯度算子
 */
class SobelOp : public IOpNode
{
public:
    SobelOp();
    ~SobelOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EdgeDetection; }
    std::string GetName() const override { return "Sobel"; }
    std::string GetDisplayName() const override { return "Sobel 梯度算子 (Sobel)"; }
    std::string GetDescription() const override { return "结合高斯平滑与微分求导的一阶梯度算子，提取水平或垂直方向边缘。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<SobelOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Scharr 边缘算子
 */
class ScharrOp : public IOpNode
{
public:
    ScharrOp();
    ~ScharrOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EdgeDetection; }
    std::string GetName() const override { return "Scharr"; }
    std::string GetDisplayName() const override { return "Scharr 高精度边缘 (Scharr)"; }
    std::string GetDescription() const override { return "比 3x3 Sobel 具备更精确梯度方向导数的微分算子。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ScharrOp>(*this); }

private:
    std::vector<OpParameter> _params;
};

/**
 * @brief Laplacian 二阶微分边缘算子
 */
class LaplacianOp : public IOpNode
{
public:
    LaplacianOp();
    ~LaplacianOp() override = default;

    OpCategory GetCategory() const override { return OpCategory::EdgeDetection; }
    std::string GetName() const override { return "Laplacian"; }
    std::string GetDisplayName() const override { return "拉普拉斯算子 (Laplacian)"; }
    std::string GetDescription() const override { return "通过二阶空间微分计算零交叉点，具有各向同性边缘检测特性。"; }
    std::string GetPrincipleDoc() const override;

    bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) override;
    std::vector<OpParameter>& GetParameters() override { return _params; }
    const std::vector<OpParameter>& GetParameters() const override { return _params; }

    std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const override;
    std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const override;
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<LaplacianOp>(*this); }

private:
    std::vector<OpParameter> _params;
};
