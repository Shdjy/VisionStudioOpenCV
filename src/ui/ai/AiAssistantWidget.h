#pragma once

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "../../ai/AiConfigManager.h"
#include "../../ai/OpenAiCompatibleProvider.h"
#include "../../ai/AiDetectionPlanner.h"
#include "../../ai/AiResponseValidator.h"
#include "../../bll/PipelineManager.h"
#include "../../detection/DetectionTaskManager.h"

namespace Ui {
class AiAssistantWidget;
}

/**
 * @brief AI 视觉算法辅助设计与流水线自动生成面板 (UI 表现层)
 */
class AiAssistantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AiAssistantWidget(
        PipelineManager* pipelineManager,
        DetectionTaskManager* taskManager,
        QWidget* parent = nullptr);
    ~AiAssistantWidget() override;

    /**
     * @brief 设置主窗口传递的当前图像 (用于多模态视觉分析)
     */
    void SetCurrentSourceImage(const cv::Mat& img);

signals:
    void DetectionTaskApplied();
    void PipelineStepsApplied();

private slots:
    void OnFetchModelsClicked();
    void OnGeneratePlanClicked();
    void OnApplyDetectionTaskClicked();
    void OnApplyPipelineClicked();
    void OnQuickTemplateCountClicked();
    void OnQuickTemplateDefectClicked();

private:
    Ui::AiAssistantWidget* ui;
    PipelineManager* _pipelineManager = nullptr;
    DetectionTaskManager* _taskManager = nullptr;

    OpenAiCompatibleProvider* _provider = nullptr;
    AiDetectionPlanner* _planner = nullptr;

    cv::Mat _currentImage;
    ValidatedAiPlan _lastPlan;

    void InitUiAndConfig();
    void SaveConfigFromUi();
};
