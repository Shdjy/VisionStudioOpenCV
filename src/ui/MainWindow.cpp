#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AboutDialog.h"
#include "MaskEditorDialog.h"
#include "detection/BatchTestDialog.h"
#include "../core/OpRegistry.h"
#include "../bll/ProjectManager.h"
#include "../utils/Logger.h"
#include "../utils/ImageIoUtils.h"
#include "../utils/IconUtils.h"
#include "../utils/AppSettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStyle>
#include <QShowEvent>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/app_icon.png"));

    InitUiAndDocks();
    SetupConnections();

    Logger::Info("MainWindow", "VisionStudio OpenCV 系统初始化完成 (含通用智能检测与 AI 引擎)");

    LoadDefaultTestImage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (_isFirstShow)
    {
        _isFirstShow = false;

#ifdef _WIN32
        // 启用 Windows 10/11 原生沉浸式暗色标题栏 (与整个 IDE 色调完美统一)
        HWND hwnd = (HWND)winId();
        if (hwnd)
        {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
        }
#endif

        // 1. 确保启动全屏最大化
        setWindowState(windowState() | Qt::WindowMaximized);

        // 2. 初始比例分配 (VS 风格黄金比例布局，默认底部折叠留出最大视觉视口)
        resizeDocks({ _dockOperators, _dockPipeline }, { 280, 280 }, Qt::Horizontal);
        resizeDocks({ _dockProperties, _dockHistogram, _dockDetection, _dockAi }, { 340, 340, 340, 340 }, Qt::Horizontal);
    }
}

void MainWindow::InitUiAndDocks()
{
    // 1. 配置 QMainWindow 现代高级停靠选项 (支持拖拽边缘自动吸附停靠、嵌套、多Tab分组)
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // 2. 中央主图像视口
    _imageViewer = new ImageViewer(this);
    ui->centralLayout->addWidget(_imageViewer);

    // 辅助配置 Lambda: 保证每个 DockWidget 均可自由脱离、任意边停靠、边缘吸附
    auto configureDock = [](QDockWidget* dock) {
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    };

    // 3. 左侧 Dock: 算子库
    _dockOperators = new QDockWidget("算子库 (Operators)", this);
    _dockOperators->setObjectName("dockOperators");
    configureDock(_dockOperators);
    _operatorTree = new OperatorTreeWidget(_dockOperators);
    _dockOperators->setWidget(_operatorTree);
    addDockWidget(Qt::LeftDockWidgetArea, _dockOperators);

    // 4. 左侧 Dock: 流水线序列
    _dockPipeline = new QDockWidget("流水线序列 (Pipeline)", this);
    _dockPipeline->setObjectName("dockPipeline");
    configureDock(_dockPipeline);
    _pipelineList = new PipelineListWidget(_dockPipeline);
    _pipelineList->SetPipelineManager(&_pipelineManager);
    _dockPipeline->setWidget(_pipelineList);
    addDockWidget(Qt::LeftDockWidgetArea, _dockPipeline);

    // 5. 右侧 Dock: 属性与原理教学
    _dockProperties = new QDockWidget("参数调节与原理 (Properties & Principles)", this);
    _dockProperties->setObjectName("dockProperties");
    configureDock(_dockProperties);
    _propertyPanel = new PropertyPanel(_dockProperties);
    _propertyPanel->SetPipelineManager(&_pipelineManager);
    _dockProperties->setWidget(_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, _dockProperties);

    // 6. 右侧 Dock: 直方图与剖面分析
    _dockHistogram = new QDockWidget("直方图与剖面分析 (Analysis)", this);
    _dockHistogram->setObjectName("dockHistogram");
    configureDock(_dockHistogram);
    _histogramWidget = new HistogramWidget(_dockHistogram);
    _dockHistogram->setWidget(_histogramWidget);
    addDockWidget(Qt::RightDockWidgetArea, _dockHistogram);

    // 7. 右侧 Dock: 智能检测任务管理 (新)
    _dockDetection = new QDockWidget("智能检测与质检 (Inspection)", this);
    _dockDetection->setObjectName("dockDetection");
    configureDock(_dockDetection);
    _detectionPanel = new DetectionPanelWidget(&_taskManager, _dockDetection);
    _dockDetection->setWidget(_detectionPanel);
    addDockWidget(Qt::RightDockWidgetArea, _dockDetection);

    // 8. 右侧 Dock: AI 智能视觉助手 (新)
    _dockAi = new QDockWidget("AI 智能视觉助手 (AI Assistant)", this);
    _dockAi->setObjectName("dockAi");
    configureDock(_dockAi);
    _aiAssistant = new AiAssistantWidget(&_pipelineManager, &_taskManager, _dockAi);
    _dockAi->setWidget(_aiAssistant);
    addDockWidget(Qt::RightDockWidgetArea, _dockAi);

    // 9. 底部 Dock: 多语言代码生成
    _dockCode = new QDockWidget("多语言代码生成 (Code Generation)", this);
    _dockCode->setObjectName("dockCode");
    configureDock(_dockCode);
    _codePreview = new CodePreviewWidget(_dockCode);
    _codePreview->SetPipelineManager(&_pipelineManager);
    _dockCode->setWidget(_codePreview);
    addDockWidget(Qt::BottomDockWidgetArea, _dockCode);
    _dockCode->hide();

    // 10. 底部 Dock: 运行日志控制台
    _dockLog = new QDockWidget("运行日志与控制台 (Log Console)", this);
    _dockLog->setObjectName("dockLog");
    configureDock(_dockLog);
    _logPanel = new LogPanelWidget(_dockLog);
    _dockLog->setWidget(_logPanel);
    addDockWidget(Qt::BottomDockWidgetArea, _dockLog);
    _dockLog->hide();

    // 11. Tab 组合与初始激活
    tabifyDockWidget(_dockProperties, _dockHistogram);
    tabifyDockWidget(_dockHistogram, _dockDetection);
    tabifyDockWidget(_dockDetection, _dockAi);
    _dockProperties->raise();

    tabifyDockWidget(_dockCode, _dockLog);

    // 12. 设置工具栏与菜单项专属图标
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui->mainToolBar->setIconSize(QSize(22, 22));

    ui->actionOpenImage->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->actionSaveImage->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->actionImportProject->setIcon(IconUtils::CreateImportIcon());
    ui->actionExportProject->setIcon(IconUtils::CreateExportIcon());
    ui->actionRunPipeline->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionToggleAllSteps->setIcon(IconUtils::CreateSelectAllIcon(QColor(76, 175, 80), 32));
    ui->actionCreateBranch->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->actionClearPipeline->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    ui->actionResetLayout->setIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    ui->actionAbout->setIcon(QIcon(":/icons/app_icon.png"));

    // 13. 工具栏快捷动作扩展
    ui->mainToolBar->addSeparator();

    auto* actionCreateMask = new QAction(style()->standardIcon(QStyle::SP_FileDialogListView), "创建/编辑交互式掩码", this);
    actionCreateMask->setToolTip("一键可视化绘制或生成局部掩码 (Alt+M)");
    actionCreateMask->setStatusTip("弹出工作台鼠标拖动选区或根据阈值自动生成局部掩码");
    actionCreateMask->setShortcut(QKeySequence("Alt+M"));
    connect(actionCreateMask, &QAction::triggered, this, &MainWindow::OnCreateMaskDialog);
    ui->mainToolBar->addAction(actionCreateMask);

    // 智能检测 Dock 快捷唤出
    auto* actionToggleDetection = new QAction(style()->standardIcon(QStyle::SP_DialogApplyButton), "智能检测与质检面板", this);
    actionToggleDetection->setCheckable(true);
    actionToggleDetection->setChecked(true);
    actionToggleDetection->setToolTip("显示/隐藏 智能检测与质检面板 (Alt+D)");
    actionToggleDetection->setStatusTip("打开目标计数与工件表面缺陷检测面板");
    actionToggleDetection->setShortcut(QKeySequence("Alt+D"));
    connect(actionToggleDetection, &QAction::triggered, this, &MainWindow::OnToggleDetectionDock);
    connect(_dockDetection, &QDockWidget::visibilityChanged, actionToggleDetection, &QAction::setChecked);
    ui->mainToolBar->addAction(actionToggleDetection);

    // AI 视觉助手 Dock 快捷唤出
    auto* actionToggleAi = new QAction(style()->standardIcon(QStyle::SP_ComputerIcon), "AI 视觉助手", this);
    actionToggleAi->setCheckable(true);
    actionToggleAi->setChecked(false);
    actionToggleAi->setToolTip("显示/隐藏 AI 智能视觉助手面板 (Alt+A)");
    actionToggleAi->setStatusTip("使用大模型自动获取模型并规划检测方案");
    actionToggleAi->setShortcut(QKeySequence("Alt+A"));
    connect(actionToggleAi, &QAction::triggered, this, &MainWindow::OnToggleAiDock);
    connect(_dockAi, &QDockWidget::visibilityChanged, actionToggleAi, &QAction::setChecked);
    ui->mainToolBar->addAction(actionToggleAi);

    // 批量测试对话框快捷入口
    auto* actionBatchTest = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "批量测试与统计评估", this);
    actionBatchTest->setToolTip("批量对整个文件夹图像执行检测并统计良品率 (Alt+B)");
    actionBatchTest->setStatusTip("批量执行质检并导出 CSV 报表");
    actionBatchTest->setShortcut(QKeySequence("Alt+B"));
    connect(actionBatchTest, &QAction::triggered, this, &MainWindow::OnShowBatchTestDialog);
    ui->mainToolBar->addAction(actionBatchTest);

    ui->mainToolBar->addSeparator();

    auto* actionToggleCode = new QAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "代码生成面板", this);
    actionToggleCode->setCheckable(true);
    actionToggleCode->setChecked(false);
    actionToggleCode->setToolTip("显示/隐藏 底部代码生成面板 (Alt+C)");
    actionToggleCode->setShortcut(QKeySequence("Alt+C"));
    connect(actionToggleCode, &QAction::triggered, this, &MainWindow::OnToggleCodeDock);
    connect(_dockCode, &QDockWidget::visibilityChanged, actionToggleCode, &QAction::setChecked);
    ui->mainToolBar->addAction(actionToggleCode);

    auto* actionToggleLog = new QAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), "运行日志面板", this);
    actionToggleLog->setCheckable(true);
    actionToggleLog->setChecked(false);
    actionToggleLog->setToolTip("显示/隐藏 底部运行日志控制台 (Alt+L)");
    actionToggleLog->setShortcut(QKeySequence("Alt+L"));
    connect(actionToggleLog, &QAction::triggered, this, &MainWindow::OnToggleLogDock);
    connect(_dockLog, &QDockWidget::visibilityChanged, actionToggleLog, &QAction::setChecked);
    ui->mainToolBar->addAction(actionToggleLog);

    // 14. 视图菜单补充 Dock 控制
    ui->menuView->addAction(_dockOperators->toggleViewAction());
    ui->menuView->addAction(_dockPipeline->toggleViewAction());
    ui->menuView->addAction(_dockProperties->toggleViewAction());
    ui->menuView->addAction(_dockHistogram->toggleViewAction());
    ui->menuView->addAction(_dockDetection->toggleViewAction());
    ui->menuView->addAction(_dockAi->toggleViewAction());
    ui->menuView->addSeparator();
    ui->menuView->addAction(actionCreateMask);
    ui->menuView->addAction(actionBatchTest);
    ui->menuView->addAction(actionToggleCode);
    ui->menuView->addAction(actionToggleLog);

    // 15. 状态栏
    _lblStatusInfo = new QLabel("就绪", this);
    _lblPipelineTime = new QLabel("流水线耗时: 0.0 ms", this);
    _lblImageDim = new QLabel("图像: 未加载", this);

    ui->statusBar->addWidget(_lblStatusInfo, 1);
    ui->statusBar->addPermanentWidget(_lblPipelineTime);
    ui->statusBar->addPermanentWidget(_lblImageDim);
}

void MainWindow::OnCreateMaskDialog()
{
    const cv::Mat& currentMat = _imageViewer->GetCurrentImage();
    const cv::Mat& sourceMat = _pipelineManager.GetSourceImage();
    if (currentMat.empty() && sourceMat.empty())
    {
        QMessageBox::warning(this, "提示", "请先加载或打开图像后再创建掩码！");
        return;
    }

    MaskEditorDialog dlg(currentMat.empty() ? sourceMat : currentMat, sourceMat, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        auto op = dlg.GetConfiguredOperator();
        if (op)
        {
            std::string dispName = op->GetDisplayName();
            int stepIdx = _pipelineManager.AddStep(std::move(op));
            _pipelineManager.Execute();
            _pipelineList->SelectStep(stepIdx);
            _propertyPanel->SetCurrentStep(stepIdx);
            Logger::Info("MainWindow", "已通过交互式掩码编辑器添加: " + dispName);
        }
    }
}

void MainWindow::OnToggleCodeDock()
{
    if (_dockCode->isHidden())
    {
        _dockCode->show();
        _dockCode->raise();
        resizeDocks({ _dockCode }, { 240 }, Qt::Vertical);
    }
    else
    {
        if (_dockCode->visibleRegion().isEmpty())
        {
            _dockCode->raise();
        }
        else
        {
            _dockCode->hide();
        }
    }
}

void MainWindow::OnToggleLogDock()
{
    if (_dockLog->isHidden())
    {
        _dockLog->show();
        _dockLog->raise();
        resizeDocks({ _dockLog }, { 240 }, Qt::Vertical);
    }
    else
    {
        if (_dockLog->visibleRegion().isEmpty())
        {
            _dockLog->raise();
        }
        else
        {
            _dockLog->hide();
        }
    }
}

void MainWindow::OnToggleDetectionDock()
{
    if (_dockDetection->isHidden())
    {
        _dockDetection->show();
        _dockDetection->raise();
    }
    else
    {
        if (_dockDetection->visibleRegion().isEmpty())
        {
            _dockDetection->raise();
        }
        else
        {
            _dockDetection->hide();
        }
    }
}

void MainWindow::OnToggleAiDock()
{
    if (_dockAi->isHidden())
    {
        _dockAi->show();
        _dockAi->raise();
    }
    else
    {
        if (_dockAi->visibleRegion().isEmpty())
        {
            _dockAi->raise();
        }
        else
        {
            _dockAi->hide();
        }
    }
}

void MainWindow::OnShowBatchTestDialog()
{
    BatchTestDialog dlg(&_taskManager, this);
    dlg.exec();
}

void MainWindow::OnDetectionResultRendered(const cv::Mat& visualization)
{
    if (!visualization.empty())
    {
        _imageViewer->SetImage(visualization, _pipelineManager.GetSourceImage());
        Logger::Info("MainWindow", "主视口已更新为智能检测判定叠加图");
    }
}

void MainWindow::SetupConnections()
{
    // 菜单与工具栏
    connect(ui->actionOpenImage, &QAction::triggered, this, &MainWindow::OnOpenImage);
    connect(ui->actionSaveImage, &QAction::triggered, this, &MainWindow::OnSaveImage);
    connect(ui->actionImportProject, &QAction::triggered, this, &MainWindow::OnImportProject);
    connect(ui->actionExportProject, &QAction::triggered, this, &MainWindow::OnExportProject);
    connect(ui->actionRunPipeline, &QAction::triggered, this, &MainWindow::OnRunPipeline);
    connect(ui->actionToggleAllSteps, &QAction::triggered, this, [this]() {
        _pipelineManager.SetAllStepsEnabled(!_pipelineManager.AreAllStepsEnabled());
    });
    connect(ui->actionCreateBranch, &QAction::triggered, this, [this]() {
        _pipelineManager.CreateBranch();
    });
    connect(ui->actionClearPipeline, &QAction::triggered, this, &MainWindow::OnClearPipeline);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::OnAbout);
    connect(ui->actionResetLayout, &QAction::triggered, this, &MainWindow::OnResetLayout);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // 算子添加与算子树点击即时预览
    connect(_operatorTree, &OperatorTreeWidget::OperatorAddRequested, this, &MainWindow::OnOperatorAddRequested);
    connect(_operatorTree, &OperatorTreeWidget::OperatorSelected, _propertyPanel, &PropertyPanel::ShowOperatorPreview);

    // 流水线与分支联动
    connect(_pipelineList, &PipelineListWidget::StepSelected, this, &MainWindow::OnStepSelected);
    connect(_pipelineList, &PipelineListWidget::BranchSelected, this, [this](int branchIdx) {
        const cv::Mat& branchOut = _pipelineManager.GetBranchFinalOutput(branchIdx);
        if (!branchOut.empty())
        {
            _imageViewer->SetImage(branchOut);
        }
    });
    connect(&_pipelineManager, &PipelineManager::ExecutionCompleted, this, &MainWindow::OnPipelineExecutionCompleted);

    // 检测面板与 AI 助手联动
    connect(_detectionPanel, &DetectionPanelWidget::DetectionExecuted, this, &MainWindow::OnDetectionResultRendered);
    connect(_detectionPanel, &DetectionPanelWidget::BatchTestRequested, this, &MainWindow::OnShowBatchTestDialog);

    connect(_aiAssistant, &AiAssistantWidget::DetectionTaskApplied, this, [this]() {
        _dockDetection->show();
        _dockDetection->raise();
    });
    connect(_aiAssistant, &AiAssistantWidget::PipelineStepsApplied, this, [this]() {
        _dockPipeline->show();
        _dockPipeline->raise();
        _pipelineManager.Execute();
    });

    // 视口与直方图/剖面联动
    connect(_imageViewer, &ImageViewer::ImageUpdated, _histogramWidget, &HistogramWidget::UpdateHistogram);
    connect(_imageViewer, &ImageViewer::LineProfileUpdated, _histogramWidget, &HistogramWidget::UpdateLineProfile);
}

void MainWindow::LoadDefaultTestImage()
{
    // 生成一张包含几何形状、渐变与模拟工件的高分辨率演示图
    cv::Mat defaultImg(600, 800, CV_8UC3, cv::Scalar(40, 40, 40));

    // 渐变底纹
    for (int r = 0; r < defaultImg.rows; ++r)
    {
        for (int c = 0; c < defaultImg.cols; ++c)
        {
            defaultImg.at<cv::Vec3b>(r, c)[0] = static_cast<uchar>(30 + 50.0 * c / defaultImg.cols);
            defaultImg.at<cv::Vec3b>(r, c)[1] = static_cast<uchar>(30 + 40.0 * r / defaultImg.rows);
            defaultImg.at<cv::Vec3b>(r, c)[2] = static_cast<uchar>(40 + 30.0 * (r + c) / (defaultImg.rows + defaultImg.cols));
        }
    }

    // 几何零件 (用于目标计数与形状分析测试)
    cv::rectangle(defaultImg, cv::Rect(50, 60, 200, 150), cv::Scalar(0, 165, 255), -1); // 橙色矩形
    cv::circle(defaultImg, cv::Point(400, 135), 75, cv::Scalar(0, 255, 128), -1);         // 绿色圆形
    cv::circle(defaultImg, cv::Point(650, 135), 70, cv::Scalar(255, 100, 0), 4);          // 蓝色圆环

    // 多边形
    std::vector<cv::Point> pts = { cv::Point(150, 480), cv::Point(80, 360), cv::Point(220, 360) };
    cv::fillPoly(defaultImg, pts, cv::Scalar(255, 255, 0));

    // 模拟 12 个瓶盖/圆形工件排列阵列 (便于开箱即测目标计数)
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            cv::Point pt(340 + col * 75, 340 + row * 65);
            cv::circle(defaultImg, pt, 24, cv::Scalar(230, 230, 230), -1);
            cv::circle(defaultImg, pt, 20, cv::Scalar(180, 180, 180), -1);
            cv::circle(defaultImg, pt, 12, cv::Scalar(120, 120, 120), -1);
        }
    }

    // 标定十字与线条
    cv::line(defaultImg, cv::Point(670, 310), cv::Point(780, 310), cv::Scalar(0, 0, 255), 2);
    cv::line(defaultImg, cv::Point(725, 260), cv::Point(725, 370), cv::Scalar(0, 0, 255), 2);

    // 文字
    cv::putText(defaultImg, "VisionStudio OpenCV Industrial Test Target", cv::Point(40, 565),
                cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(255, 255, 255), 2);

    _pipelineManager.SetSourceImage(defaultImg);
    _imageViewer->SetImage(defaultImg, defaultImg);

    if (_detectionPanel)
    {
        _detectionPanel->SetSourceImage(defaultImg);
    }
    if (_aiAssistant)
    {
        _aiAssistant->SetCurrentSourceImage(defaultImg);
    }

    _lblImageDim->setText(QString("图像: %1x%2 (内置工业测试靶标)").arg(defaultImg.cols).arg(defaultImg.rows));
    Logger::Info("MainWindow", "已自动加载内置工业测试靶标 (含 12 个排列工件)");
}

void MainWindow::OnOpenImage()
{
    QString initialDir = AppSettings::GetLastDirectory(AppSettings::KeyImageDir);
    QString filePath = QFileDialog::getOpenFileName(this, "打开图像文件", initialDir, "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    AppSettings::SetLastDirectory(AppSettings::KeyImageDir, filePath);

    std::string ioError;
    cv::Mat img = ImageIoUtils::ReadImage(filePath.toUtf8().constData(), cv::IMREAD_COLOR, ioError);
    if (img.empty())
    {
        QMessageBox::critical(this, "打开失败", "无法读取该图像文件: " + filePath);
        Logger::Error("MainWindow", "读取图像失败: " + ioError);
        return;
    }

    _pipelineManager.SetSourceImage(img);
    _imageViewer->SetImage(img, img);
    _lblImageDim->setText(QString("图像: %1x%2").arg(img.cols).arg(img.rows));

    if (_detectionPanel)
    {
        _detectionPanel->SetSourceImage(img);
    }
    if (_aiAssistant)
    {
        _aiAssistant->SetCurrentSourceImage(img);
    }

    Logger::Info("MainWindow", "成功打开图像: " + filePath.toStdString());

    _pipelineManager.Execute(true);
}

void MainWindow::OnSaveImage()
{
    const cv::Mat& outputMat = _pipelineManager.GetFinalOutput();
    if (outputMat.empty())
    {
        QMessageBox::warning(this, "提示", "当前无有效图像可保存！");
        return;
    }

    QString defaultPath = AppSettings::GetDefaultFilePath(AppSettings::KeyImageDir, "processed_result.png");
    QString filePath = QFileDialog::getSaveFileName(this, "保存处理结果图像", defaultPath, "PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    AppSettings::SetLastDirectory(AppSettings::KeyImageDir, filePath);

    std::string ioError;
    if (ImageIoUtils::WriteImage(filePath.toUtf8().constData(), outputMat, ioError))
    {
        QMessageBox::information(this, "保存成功", "图像已成功保存至:\n" + filePath);
        Logger::Info("MainWindow", "图像已保存: " + filePath.toStdString());
    }
    else
    {
        QMessageBox::critical(this, "保存失败", "无法写入图像文件！");
        Logger::Error("MainWindow", "写入图像文件失败: " + ioError);
    }
}

void MainWindow::OnImportProject()
{
    QString initialDir = AppSettings::GetLastDirectory(AppSettings::KeyProjectDir);
    QString filePath = QFileDialog::getOpenFileName(this, "导入工程配置", initialDir, "VisionStudio Project (*.json *.vsproj);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    AppSettings::SetLastDirectory(AppSettings::KeyProjectDir, filePath);

    QString err;
    if (ProjectManager::LoadProject(_pipelineManager, &_taskManager, filePath, err))
    {
        QMessageBox::information(this, "导入成功", "工程配置已成功加载 (含流水线与检测任务)！");
        Logger::Info("MainWindow", "已导入工程配置: " + filePath.toStdString());
    }
    else
    {
        QMessageBox::critical(this, "导入失败", err);
        Logger::Error("MainWindow", "工程配置导入失败: " + err.toStdString());
    }
}

void MainWindow::OnExportProject()
{
    if (_pipelineManager.GetStepCount() == 0 && _taskManager.GetTaskCount() == 0)
    {
        QMessageBox::warning(this, "提示", "当前流水线与检测任务均为空，无法导出工程配置！");
        return;
    }

    QString defaultPath = AppSettings::GetDefaultFilePath(AppSettings::KeyProjectDir, "pipeline_project.json");
    QString filePath = QFileDialog::getSaveFileName(this, "导出工程配置", defaultPath, "VisionStudio Project (*.json *.vsproj);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    AppSettings::SetLastDirectory(AppSettings::KeyProjectDir, filePath);

    QString err;
    if (ProjectManager::SaveProject(_pipelineManager, &_taskManager, filePath, err))
    {
        QMessageBox::information(this, "导出成功", "工程配置已保存至:\n" + filePath);
        Logger::Info("MainWindow", "已导出工程配置: " + filePath.toStdString());
    }
    else
    {
        QMessageBox::critical(this, "导出失败", err);
        Logger::Error("MainWindow", "工程配置导出失败: " + err.toStdString());
    }
}

void MainWindow::OnRunPipeline()
{
    Logger::Info("MainWindow", "手动触发执行整条流水线...");
    _pipelineManager.Execute(true);
}

void MainWindow::OnClearPipeline()
{
    _pipelineList->OnClearClicked();
}

void MainWindow::OnAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::OnResetLayout()
{
    _dockOperators->show();
    _dockPipeline->show();
    _dockProperties->show();
    _dockHistogram->show();
    _dockDetection->show();
    _dockAi->show();
    _dockCode->show();
    _dockLog->show();

    resizeDocks({ _dockOperators, _dockPipeline }, { 280, 280 }, Qt::Horizontal);
    resizeDocks({ _dockProperties, _dockHistogram, _dockDetection, _dockAi }, { 340, 340, 340, 340 }, Qt::Horizontal);
    resizeDocks({ _dockCode, _dockLog }, { 220, 220 }, Qt::Vertical);
}

void MainWindow::OnOperatorAddRequested(const QString& opName)
{
    auto op = OpRegistry::Instance().Create(opName.toStdString());
    if (op)
    {
        std::string dispName = op->GetDisplayName();
        int stepIdx = _pipelineManager.AddStep(std::move(op));
        _pipelineManager.Execute();
        _pipelineList->SelectStep(stepIdx);
        _propertyPanel->SetCurrentStep(stepIdx);
        Logger::Info("MainWindow", "流水线添加算子: " + dispName);
    }
}

void MainWindow::OnStepSelected(int index)
{
    _propertyPanel->SetCurrentStep(index);

    if (index >= 0 && index < _pipelineManager.GetStepCount())
    {
        const cv::Mat& stepOut = _pipelineManager.GetStepOutput(index);
        _imageViewer->SetImage(stepOut, _pipelineManager.GetSourceImage());
    }
    else
    {
        _imageViewer->SetImage(_pipelineManager.GetFinalOutput(), _pipelineManager.GetSourceImage());
    }
}

void MainWindow::OnPipelineExecutionCompleted(double totalTimeMs)
{
    _lblPipelineTime->setText(QString("流水线耗时: %1 ms").arg(QString::number(totalTimeMs, 'f', 1)));

    int currentSelected = _pipelineList->GetCurrentSelectedIndex();
    if (currentSelected >= 0 && currentSelected < _pipelineManager.GetStepCount())
    {
        const cv::Mat& stepOut = _pipelineManager.GetStepOutput(currentSelected);
        _imageViewer->SetImage(stepOut, _pipelineManager.GetSourceImage());
    }
    else
    {
        _imageViewer->SetImage(_pipelineManager.GetFinalOutput(), _pipelineManager.GetSourceImage());
    }
}
