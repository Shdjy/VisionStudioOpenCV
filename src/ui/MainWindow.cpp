#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AboutDialog.h"
#include "../core/OpRegistry.h"
#include "../bll/ProjectManager.h"
#include "../utils/Logger.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    InitUiAndDocks();
    SetupConnections();

    Logger::Info("MainWindow", "VisionStudio OpenCV 系统初始化完成");

    LoadDefaultTestImage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::InitUiAndDocks()
{
    // 1. 中央主视口
    _imageViewer = new ImageViewer(this);
    ui->centralLayout->addWidget(_imageViewer);

    // 2. 左侧 Dock: 算子库
    _dockOperators = new QDockWidget("算子库 (Operators)", this);
    _dockOperators->setObjectName("dockOperators");
    _operatorTree = new OperatorTreeWidget(_dockOperators);
    _dockOperators->setWidget(_operatorTree);
    addDockWidget(Qt::LeftDockWidgetArea, _dockOperators);

    // 3. 左侧 Dock: 流水线
    _dockPipeline = new QDockWidget("流水线序列 (Pipeline)", this);
    _dockPipeline->setObjectName("dockPipeline");
    _pipelineList = new PipelineListWidget(_dockPipeline);
    _pipelineList->SetPipelineManager(&_pipelineManager);
    _dockPipeline->setWidget(_pipelineList);
    addDockWidget(Qt::LeftDockWidgetArea, _dockPipeline);

    // 4. 右侧 Dock: 属性与原理教学
    _dockProperties = new QDockWidget("参数调节与原理 (Properties & Principles)", this);
    _dockProperties->setObjectName("dockProperties");
    _propertyPanel = new PropertyPanel(_dockProperties);
    _propertyPanel->SetPipelineManager(&_pipelineManager);
    _dockProperties->setWidget(_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, _dockProperties);

    // 5. 右侧 Dock: 直方图分析
    _dockHistogram = new QDockWidget("直方图分析 (Histogram)", this);
    _dockHistogram->setObjectName("dockHistogram");
    _histogramWidget = new HistogramWidget(_dockHistogram);
    _dockHistogram->setWidget(_histogramWidget);
    addDockWidget(Qt::RightDockWidgetArea, _dockHistogram);

    // 6. 底部 Dock: 代码生成 (默认停靠底部并折叠/隐藏，需要时唤出)
    _dockCode = new QDockWidget("多语言代码生成 (Code Generation)", this);
    _dockCode->setObjectName("dockCode");
    _codePreview = new CodePreviewWidget(_dockCode);
    _codePreview->SetPipelineManager(&_pipelineManager);
    _dockCode->setWidget(_codePreview);
    addDockWidget(Qt::BottomDockWidgetArea, _dockCode);
    _dockCode->hide(); // 默认隐藏，扩大视口区域

    // 7. 底部 Dock: 日志控制台 (默认停靠底部并折叠/隐藏)
    _dockLog = new QDockWidget("运行日志与控制台 (Log Console)", this);
    _dockLog->setObjectName("dockLog");
    _logPanel = new LogPanelWidget(_dockLog);
    _dockLog->setWidget(_logPanel);
    addDockWidget(Qt::BottomDockWidgetArea, _dockLog);
    _dockLog->hide(); // 默认隐藏

    // 拆分与堆叠优化
    tabifyDockWidget(_dockCode, _dockLog);

    tabifyDockWidget(_dockProperties, _dockHistogram);
    _dockProperties->raise();

    // 视图菜单增加各 Dock 控制
    ui->menuView->addAction(_dockOperators->toggleViewAction());
    ui->menuView->addAction(_dockPipeline->toggleViewAction());
    ui->menuView->addAction(_dockProperties->toggleViewAction());
    ui->menuView->addAction(_dockHistogram->toggleViewAction());
    ui->menuView->addSeparator();
    ui->menuView->addAction(_dockCode->toggleViewAction());
    ui->menuView->addAction(_dockLog->toggleViewAction());

    // 主工具栏增加代码生成与日志控制台快捷显隐按钮
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(_dockCode->toggleViewAction());
    ui->mainToolBar->addAction(_dockLog->toggleViewAction());

    // 状态栏
    _lblStatusInfo = new QLabel("就绪", this);
    _lblPipelineTime = new QLabel("流水线耗时: 0.0 ms", this);
    _lblImageDim = new QLabel("图像: 未加载", this);

    ui->statusBar->addWidget(_lblStatusInfo, 1);
    ui->statusBar->addPermanentWidget(_lblPipelineTime);
    ui->statusBar->addPermanentWidget(_lblImageDim);
}

void MainWindow::SetupConnections()
{
    // 菜单与工具栏
    connect(ui->actionOpenImage, &QAction::triggered, this, &MainWindow::OnOpenImage);
    connect(ui->actionSaveImage, &QAction::triggered, this, &MainWindow::OnSaveImage);
    connect(ui->actionImportProject, &QAction::triggered, this, &MainWindow::OnImportProject);
    connect(ui->actionExportProject, &QAction::triggered, this, &MainWindow::OnExportProject);
    connect(ui->actionRunPipeline, &QAction::triggered, this, &MainWindow::OnRunPipeline);
    connect(ui->actionClearPipeline, &QAction::triggered, this, &MainWindow::OnClearPipeline);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::OnAbout);
    connect(ui->actionResetLayout, &QAction::triggered, this, &MainWindow::OnResetLayout);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // 算子添加
    connect(_operatorTree, &OperatorTreeWidget::OperatorAddRequested, this, &MainWindow::OnOperatorAddRequested);

    // 流水线联动
    connect(_pipelineList, &PipelineListWidget::StepSelected, this, &MainWindow::OnStepSelected);
    connect(&_pipelineManager, &PipelineManager::ExecutionCompleted, this, &MainWindow::OnPipelineExecutionCompleted);

    // 视口与直方图联动
    connect(_imageViewer, &ImageViewer::ImageUpdated, _histogramWidget, &HistogramWidget::UpdateHistogram);
}

void MainWindow::LoadDefaultTestImage()
{
    // 生成一张包含几何形状、渐变与文字的高分辨率演示图
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

    // 几何图形
    cv::rectangle(defaultImg, cv::Rect(50, 60, 200, 150), cv::Scalar(0, 165, 255), -1); // 橙色矩形
    cv::circle(defaultImg, cv::Point(400, 135), 75, cv::Scalar(0, 255, 128), -1);         // 绿色圆形
    cv::circle(defaultImg, cv::Point(650, 135), 70, cv::Scalar(255, 100, 0), 4);          // 蓝色圆环

    // 多边形
    std::vector<cv::Point> pts = { cv::Point(150, 480), cv::Point(80, 360), cv::Point(220, 360) };
    cv::fillPoly(defaultImg, pts, cv::Scalar(255, 255, 0));

    // 棋盘格与网格
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if ((i + j) % 2 == 0)
            {
                cv::rectangle(defaultImg, cv::Rect(320 + i * 35, 320 + j * 35, 35, 35), cv::Scalar(220, 220, 220), -1);
            }
        }
    }

    // 标定十字与线条
    cv::line(defaultImg, cv::Point(550, 350), cv::Point(750, 350), cv::Scalar(0, 0, 255), 2);
    cv::line(defaultImg, cv::Point(650, 250), cv::Point(650, 450), cv::Scalar(0, 0, 255), 2);
    cv::line(defaultImg, cv::Point(570, 270), cv::Point(730, 430), cv::Scalar(255, 0, 255), 2);

    // 文字
    cv::putText(defaultImg, "VisionStudio OpenCV Test Target", cv::Point(50, 560), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

    _pipelineManager.SetSourceImage(defaultImg);
    _imageViewer->SetImage(defaultImg, defaultImg);

    _lblImageDim->setText(QString("图像: %1x%2 (内置测试靶标)").arg(defaultImg.cols).arg(defaultImg.rows));
    Logger::Info("MainWindow", "已自动加载内置演示标定靶标图像");
}

void MainWindow::OnOpenImage()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开图像文件", "", "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    cv::Mat img = cv::imread(filePath.toLocal8Bit().toStdString());
    if (img.empty())
    {
        QMessageBox::critical(this, "打开失败", "无法读取该图像文件: " + filePath);
        Logger::Error("MainWindow", "读取图像失败: " + filePath.toStdString());
        return;
    }

    _pipelineManager.SetSourceImage(img);
    _imageViewer->SetImage(img, img);
    _lblImageDim->setText(QString("图像: %1x%2").arg(img.cols).arg(img.rows));

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

    QString filePath = QFileDialog::getSaveFileName(this, "保存处理结果图像", "processed_result.png", "PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    if (cv::imwrite(filePath.toLocal8Bit().toStdString(), outputMat))
    {
        QMessageBox::information(this, "保存成功", "图像已成功保存至:\n" + filePath);
        Logger::Info("MainWindow", "图像已保存: " + filePath.toStdString());
    }
    else
    {
        QMessageBox::critical(this, "保存失败", "无法写入图像文件！");
        Logger::Error("MainWindow", "写入图像文件失败: " + filePath.toStdString());
    }
}

void MainWindow::OnImportProject()
{
    QString filePath = QFileDialog::getOpenFileName(this, "导入工程配置", "", "VisionStudio Project (*.json *.vsproj);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    QString err;
    if (ProjectManager::LoadProject(_pipelineManager, filePath, err))
    {
        QMessageBox::information(this, "导入成功", "工程配置已成功加载！");
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
    if (_pipelineManager.GetStepCount() == 0)
    {
        QMessageBox::warning(this, "提示", "当前流水线为空，无法导出工程配置！");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "导出工程配置", "pipeline_project.json", "VisionStudio Project (*.json *.vsproj);;All Files (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }

    QString err;
    if (ProjectManager::SaveProject(_pipelineManager, filePath, err))
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
    _dockCode->show();
    _dockLog->show();
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
