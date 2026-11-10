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
 * @brief 现代化智能 Agent 对话交互面板 (UI 表现层)
 * 
 * 核心特性：
 * 1. 类似 Antigravity / DeepSeek Agent 的消息流式对话架构
 * 2. 独立支持中途输出模型推理与思考思维链 (Thinking Process 折叠/展开)
 * 3. 顶部类似 CC Switch 的服务商 Profile 快速切换与独立设置入口
 * 4. 视觉方案结构化解析与一键动作卡片执行 (应用为检测任务、流水线自动编排)
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
    void OnOpenSettingsClicked();
    void OnProviderChanged(int index);
    void OnModelChanged(const QString& model);
    void OnClearChatClicked();
    void OnGeneratePlanClicked();
    void OnApplyDetectionTaskClicked();
    void OnApplyPipelineClicked();
    void OnToggleThinkingClicked();
    void OnQuickTemplateCountClicked();
    void OnQuickTemplateDefectClicked();
    void OnQuickTemplateMeasureClicked();

private:
    Ui::AiAssistantWidget* ui;
    PipelineManager* _pipelineManager = nullptr;
    DetectionTaskManager* _taskManager = nullptr;

    OpenAiCompatibleProvider* _provider = nullptr;
    AiDetectionPlanner* _planner = nullptr;

    cv::Mat _currentImage;
    ValidatedAiPlan _lastPlan;
    bool _showThinking = true;
    QString _accumulatedChatHtml;

    void InitUiAndConfig();
    void RefreshProviderListFromConfig();
    void AppendUserMessage(const QString& text);
    void AppendAgentMessage(
        const QString& modelName,
        const QString& thinkingProcess,
        const QString& markdownSolution,
        bool isSuccess,
        const QString& errorMessage = QString());
    void UpdateActionCardStatus();
    QString FormatMarkdownToHtml(const QString& markdown);
};
