#include "DetectionPanelWidget.h"
#include "ui_DetectionPanelWidget.h"
#include "../../utils/Logger.h"
#include <QMessageBox>

DetectionPanelWidget::DetectionPanelWidget(
    DetectionTaskManager* taskManager,
    QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DetectionPanelWidget)
    , _taskManager(taskManager)
{
    ui->setupUi(this);

    // 设置表格表头
    ui->tableResults->setColumnCount(6);
    ui->tableResults->setHorizontalHeaderLabels({ "序号", "类别", "位置 (X, Y, W, H)", "面积/尺寸 (px)", "置信度", "判定" });
    ui->tableResults->horizontalHeader()->setStretchLastSection(true);

    if (_taskManager)
    {
        connect(_taskManager, &DetectionTaskManager::TaskListChanged, this, &DetectionPanelWidget::OnTaskListChanged);
        connect(_taskManager, &DetectionTaskManager::ActiveTaskChanged, this, &DetectionPanelWidget::OnTaskSelectionChanged);
        RefreshTaskComboBox();
    }

    connect(ui->cbTasks, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetectionPanelWidget::OnTaskSelectionChanged);
    connect(ui->btnNewCountTask, &QPushButton::clicked, this, &DetectionPanelWidget::OnNewCountTaskClicked);
    connect(ui->btnNewDefectTask, &QPushButton::clicked, this, &DetectionPanelWidget::OnNewDefectTaskClicked);
    connect(ui->btnDeleteTask, &QPushButton::clicked, this, &DetectionPanelWidget::OnDeleteTaskClicked);
    connect(ui->btnRun, &QPushButton::clicked, this, &DetectionPanelWidget::OnRunClicked);
    connect(ui->btnBatchTest, &QPushButton::clicked, this, &DetectionPanelWidget::OnBatchTestClicked);
}

DetectionPanelWidget::~DetectionPanelWidget()
{
    delete ui;
}

void DetectionPanelWidget::SetSourceImage(const cv::Mat& img)
{
    _sourceImage = img;
}

void DetectionPanelWidget::RefreshTaskComboBox()
{
    if (!_taskManager)
    {
        return;
    }

    ui->cbTasks->blockSignals(true);
    ui->cbTasks->clear();
    for (int i = 0; i < _taskManager->GetTaskCount(); ++i)
    {
        const auto* t = _taskManager->GetTask(i);
        if (t)
        {
            ui->cbTasks->addItem(QString::fromStdString(t->Name));
        }
    }
    int activeIdx = _taskManager->GetActiveTaskIndex();
    if (activeIdx >= 0 && activeIdx < ui->cbTasks->count())
    {
        ui->cbTasks->setCurrentIndex(activeIdx);
    }
    ui->cbTasks->blockSignals(false);

    LoadTaskToUi(_taskManager->GetActiveTask());
}

void DetectionPanelWidget::OnTaskListChanged()
{
    RefreshTaskComboBox();
}

void DetectionPanelWidget::OnTaskSelectionChanged(int index)
{
    if (_taskManager && index >= 0 && index < _taskManager->GetTaskCount())
    {
        _taskManager->SetActiveTaskIndex(index);
        LoadTaskToUi(_taskManager->GetTask(index));
    }
}

void DetectionPanelWidget::LoadTaskToUi(const DetectionTask* task)
{
    if (!task)
    {
        return;
    }

    QJsonObject cfg = task->Configuration;

    // 阈值方式
    QString tm = cfg["thresholdMethod"].toString("Otsu");
    int tmIdx = ui->cbThreshMethod->findText(tm, Qt::MatchStartsWith);
    if (tmIdx >= 0)
    {
        ui->cbThreshMethod->setCurrentIndex(tmIdx);
    }

    // 阈值/灵敏度
    if (task->Type == DetectionTaskType::ObjectCounting)
    {
        ui->sbThreshold->setValue(cfg["manualThreshold"].toInt(128));
        ui->sbMinArea->setValue(cfg["minArea"].toDouble(30.0));
    }
    else if (task->Type == DetectionTaskType::DefectDetection)
    {
        ui->sbThreshold->setValue(cfg["diffThreshold"].toInt(25));
        ui->sbMinArea->setValue(cfg["minDefectArea"].toDouble(15.0));
    }

    // 规则参数
    for (int i = 0; i < task->Rules.size(); ++i)
    {
        QJsonObject r = task->Rules[i].toObject();
        if (r.contains("expectedCount"))
        {
            ui->sbExpectedCount->setValue(r["expectedCount"].toInt(12));
        }
        if (r.contains("maxCount") && (r["ruleType"].toString() == "defect" || r.contains("defectClass")))
        {
            ui->sbMaxDefects->setValue(r["maxCount"].toInt(0));
        }
    }
}

void DetectionPanelWidget::SaveUiToTask(DetectionTask* task)
{
    if (!task)
    {
        return;
    }

    QJsonObject cfg = task->Configuration;
    QString tmText = ui->cbThreshMethod->currentText();
    if (tmText.startsWith("Otsu"))
    {
        cfg["thresholdMethod"] = "Otsu";
    }
    else if (tmText.startsWith("Adaptive"))
    {
        cfg["thresholdMethod"] = "Adaptive";
    }
    else
    {
        cfg["thresholdMethod"] = "Manual";
    }

    if (task->Type == DetectionTaskType::ObjectCounting)
    {
        cfg["manualThreshold"] = ui->sbThreshold->value();
        cfg["minArea"] = ui->sbMinArea->value();
    }
    else if (task->Type == DetectionTaskType::DefectDetection)
    {
        cfg["diffThreshold"] = ui->sbThreshold->value();
        cfg["minDefectArea"] = ui->sbMinArea->value();
    }
    task->Configuration = cfg;

    // 同步规则
    QJsonArray rules = task->Rules;
    for (int i = 0; i < rules.size(); ++i)
    {
        QJsonObject r = rules[i].toObject();
        if (r.contains("expectedCount"))
        {
            r["expectedCount"] = ui->sbExpectedCount->value();
            rules[i] = r;
        }
        if (r.contains("maxCount") && (r["ruleType"].toString() == "defect" || r.contains("defectClass")))
        {
            r["maxCount"] = ui->sbMaxDefects->value();
            rules[i] = r;
        }
    }
    task->Rules = rules;
}

void DetectionPanelWidget::OnNewCountTaskClicked()
{
    if (_taskManager)
    {
        int newIdx = _taskManager->AddTask(DetectionTaskManager::CreateDefaultCountTask(12));
        _taskManager->SetActiveTaskIndex(newIdx);
    }
}

void DetectionPanelWidget::OnNewDefectTaskClicked()
{
    if (_taskManager)
    {
        int newIdx = _taskManager->AddTask(DetectionTaskManager::CreateDefaultDefectTask());
        _taskManager->SetActiveTaskIndex(newIdx);
    }
}

void DetectionPanelWidget::OnDeleteTaskClicked()
{
    if (_taskManager && _taskManager->GetTaskCount() > 1)
    {
        _taskManager->RemoveTask(_taskManager->GetActiveTaskIndex());
    }
    else
    {
        QMessageBox::information(this, "提示", "系统中至少需保留一个检测任务模板。");
    }
}

void DetectionPanelWidget::OnRunClicked()
{
    RunCurrentDetection();
}

void DetectionPanelWidget::RunCurrentDetection()
{
    if (_sourceImage.empty())
    {
        QMessageBox::warning(this, "警告", "当前主视图未加载有效图像，请先打开或拍摄图像！");
        return;
    }

    if (!_taskManager)
    {
        return;
    }

    DetectionTask* activeTask = _taskManager->GetActiveTask();
    if (!activeTask)
    {
        return;
    }

    // 将界面修改的值同步至当前任务
    SaveUiToTask(activeTask);

    DetectionResult result;
    InspectionDecision decision;
    std::string errMsg;

    bool ok = DetectionExecutor::ExecuteTask(_sourceImage, *activeTask, result, decision, errMsg);
    if (!ok)
    {
        ui->lblVerdict->setStyleSheet("font-size: 16px; font-weight: bold; padding: 6px; border-radius: 4px; background-color: #b71c1c; color: white;");
        ui->lblVerdict->setText("执行错误 (ERROR)");
        ui->lblVerdictReasons->setText(QString::fromStdString(errMsg));
        return;
    }

    DisplayResult(result, decision);

    emit DetectionExecuted(result.Visualization);
}

void DetectionPanelWidget::DisplayResult(const DetectionResult& result, const InspectionDecision& decision)
{
    // 更新判定横幅
    if (decision.IsOK())
    {
        ui->lblVerdict->setStyleSheet("font-size: 18px; font-weight: bold; padding: 6px; border-radius: 4px; background-color: #2e7d32; color: white;");
        ui->lblVerdict->setText("✔ 判定合格 [ PASS / OK ]");
    }
    else
    {
        ui->lblVerdict->setStyleSheet("font-size: 18px; font-weight: bold; padding: 6px; border-radius: 4px; background-color: #c62828; color: white;");
        ui->lblVerdict->setText("✘ 判定不合格 [ FAIL / NG ]");
    }

    QString reasonsText = "耗时: " + QString::number(result.ExecutionTimeMs, 'f', 1) + " ms\n";
    for (const auto& r : decision.Reasons)
    {
        reasonsText += "• " + QString::fromStdString(r) + "\n";
    }
    ui->lblVerdictReasons->setText(reasonsText.trimmed());

    // 填充明细表格
    ui->tableResults->setRowCount(0);

    // 目标项
    for (size_t i = 0; i < result.Objects.size(); ++i)
    {
        const auto& obj = result.Objects[i];
        int row = ui->tableResults->rowCount();
        ui->tableResults->insertRow(row);

        ui->tableResults->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
        ui->tableResults->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(obj.Label)));
        ui->tableResults->setItem(row, 2, new QTableWidgetItem(
            QString("(%1, %2, %3, %4)").arg(static_cast<int>(obj.Rect.x))
                                      .arg(static_cast<int>(obj.Rect.y))
                                      .arg(static_cast<int>(obj.Rect.width))
                                      .arg(static_cast<int>(obj.Rect.height))));
        ui->tableResults->setItem(row, 3, new QTableWidgetItem(QString::number(obj.Area, 'f', 1)));
        ui->tableResults->setItem(row, 4, new QTableWidgetItem(QString::number(obj.Confidence, 'f', 2)));
        ui->tableResults->setItem(row, 5, new QTableWidgetItem("OK"));
    }

    // 缺陷项
    for (size_t i = 0; i < result.Defects.size(); ++i)
    {
        const auto& d = result.Defects[i];
        int row = ui->tableResults->rowCount();
        ui->tableResults->insertRow(row);

        ui->tableResults->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        ui->tableResults->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(d.DefectDisplayName)));
        ui->tableResults->setItem(row, 2, new QTableWidgetItem(
            QString("(%1, %2, %3, %4)").arg(d.BoundingBox.x)
                                      .arg(d.BoundingBox.y)
                                      .arg(d.BoundingBox.width)
                                      .arg(d.BoundingBox.height)));
        ui->tableResults->setItem(row, 3, new QTableWidgetItem(QString::number(d.Area, 'f', 1)));
        ui->tableResults->setItem(row, 4, new QTableWidgetItem(QString::number(d.Confidence, 'f', 2)));

        auto* statusItem = new QTableWidgetItem(d.IsNG ? "NG (超标)" : "OK (容限内)");
        if (d.IsNG)
        {
            statusItem->setForeground(QBrush(QColor(220, 20, 20)));
        }
        ui->tableResults->setItem(row, 5, statusItem);
    }
}

void DetectionPanelWidget::OnBatchTestClicked()
{
    emit BatchTestRequested();
}
