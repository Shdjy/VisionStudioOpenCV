#pragma once

#include "IAiProvider.h"
#include "AiResponseValidator.h"
#include <opencv2/opencv.hpp>
#include <QObject>
#include <functional>

/**
 * @brief AI 视觉检测规划器 (业务服务层 / BLL)
 * 
 * 职责：
 * 编排用户自然语言需求与工件图像，调用 AI 服务商，解析并严格验证输出，构建安全可执行的任务方案。
 */
class AiDetectionPlanner : public QObject
{
    Q_OBJECT

public:
    explicit AiDetectionPlanner(IAiProvider* provider, QObject* parent = nullptr);
    ~AiDetectionPlanner() override = default;

    void SetProvider(IAiProvider* provider) { _provider = provider; }

    /**
     * @brief 异步规划检测方案与流水线
     * @param userRequirement 用户需求自然语言描述
     * @param image 待分析工件图像 (可选，若不为空将以多模态 Base64 传递)
     * @param model 使用的模型标识名
     * @param temperature 采样温度
     * @param callback 完成回调 (ValidatedAiPlan)
     */
    void PlanDetectionAsync(
        const QString& userRequirement,
        const cv::Mat& image,
        const QString& model,
        double temperature,
        std::function<void(const ValidatedAiPlan& plan)> callback);

private:
    IAiProvider* _provider = nullptr;
    QString EncodeImageToBase64(const cv::Mat& img) const;
};
