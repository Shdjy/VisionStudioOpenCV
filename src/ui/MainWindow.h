#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QLabel>
#include "../bll/PipelineManager.h"
#include "ImageViewer.h"
#include "OperatorTreeWidget.h"
#include "PipelineListWidget.h"
#include "PropertyPanel.h"
#include "CodePreviewWidget.h"
#include "LogPanelWidget.h"
#include "HistogramWidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void OnOpenImage();
    void OnSaveImage();
    void OnImportProject();
    void OnExportProject();
    void OnRunPipeline();
    void OnClearPipeline();
    void OnAbout();
    void OnResetLayout();

    void OnToggleCodeDock();
    void OnToggleLogDock();

    void OnOperatorAddRequested(const QString& opName);
    void OnStepSelected(int index);
    void OnPipelineExecutionCompleted(double totalTimeMs);
    void OnCreateMaskDialog();

private:
    Ui::MainWindow* ui;

    // 业务与调度核心
    PipelineManager _pipelineManager;

    // 界面核心组件
    ImageViewer* _imageViewer = nullptr;
    OperatorTreeWidget* _operatorTree = nullptr;
    PipelineListWidget* _pipelineList = nullptr;
    PropertyPanel* _propertyPanel = nullptr;
    CodePreviewWidget* _codePreview = nullptr;
    LogPanelWidget* _logPanel = nullptr;
    HistogramWidget* _histogramWidget = nullptr;

    // 停靠窗口
    QDockWidget* _dockOperators = nullptr;
    QDockWidget* _dockPipeline = nullptr;
    QDockWidget* _dockProperties = nullptr;
    QDockWidget* _dockHistogram = nullptr;
    QDockWidget* _dockCode = nullptr;
    QDockWidget* _dockLog = nullptr;

    // 状态栏标签
    QLabel* _lblStatusInfo = nullptr;
    QLabel* _lblPipelineTime = nullptr;
    QLabel* _lblImageDim = nullptr;

    bool _isFirstShow = true;

    void InitUiAndDocks();
    void SetupConnections();
    void LoadDefaultTestImage();
};
