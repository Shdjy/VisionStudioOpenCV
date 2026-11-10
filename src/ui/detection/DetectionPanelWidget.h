#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <opencv2/opencv.hpp>
#include "../../detection/DetectionTaskManager.h"
#include "../../detection/DetectionExecutor.h"

namespace Ui {
class DetectionPanelWidget;
}

/**
 * @brief 工业检测任务管理与结果展示面板 (UI 表现层)
 */
class DetectionPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetectionPanelWidget(
        DetectionTaskManager* taskManager,
        QWidget* parent = nullptr);
    ~DetectionPanelWidget() override;

    /**
     * @brief 设置当前主图像源
     */
    void SetSourceImage(const cv::Mat& img);

    /**
     * @brief 执行当前选中的检测任务
     */
    void RunCurrentDetection();

signals:
    void DetectionExecuted(const cv::Mat& visualization);
    void BatchTestRequested();

private slots:
    void OnTaskSelectionChanged(int index);
    void OnNewCountTaskClicked();
    void OnNewDefectTaskClicked();
    void OnDeleteTaskClicked();
    void OnRunClicked();
    void OnBatchTestClicked();
    void OnTaskListChanged();

private:
    Ui::DetectionPanelWidget* ui;
    DetectionTaskManager* _taskManager = nullptr;
    cv::Mat _sourceImage;

    void RefreshTaskComboBox();
    void LoadTaskToUi(const DetectionTask* task);
    void SaveUiToTask(DetectionTask* task);
    void DisplayResult(const DetectionResult& result, const InspectionDecision& decision);
};
