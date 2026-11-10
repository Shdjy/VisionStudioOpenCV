#include "BatchTestDialog.h"
#include "ui_BatchTestDialog.h"
#include "../../detection/DetectionExecutor.h"
#include "../../utils/ImageIoUtils.h"
#include "../../utils/AppSettings.h"
#include "../../utils/Logger.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>

BatchTestDialog::BatchTestDialog(
    DetectionTaskManager* taskManager,
    QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::BatchTestDialog)
    , _taskManager(taskManager)
{
    ui->setupUi(this);

    ui->tableBatch->setColumnCount(6);
    ui->tableBatch->setHorizontalHeaderLabels({ "图像文件名", "判定状态", "目标数量", "缺陷数量", "耗时 (ms)", "判定原因" });
    ui->tableBatch->horizontalHeader()->setStretchLastSection(true);

    connect(ui->btnBrowseFolder, &QPushButton::clicked, this, &BatchTestDialog::OnBrowseFolder);
    connect(ui->btnStart, &QPushButton::clicked, this, &BatchTestDialog::OnStartBatch);
    connect(ui->btnStop, &QPushButton::clicked, this, &BatchTestDialog::OnStopBatch);
    connect(ui->btnExportCsv, &QPushButton::clicked, this, &BatchTestDialog::OnExportCsv);
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

BatchTestDialog::~BatchTestDialog()
{
    delete ui;
}

void BatchTestDialog::OnBrowseFolder()
{
    QString lastDir = AppSettings::GetLastDirectory(AppSettings::KeyImageDir);
    QString dir = QFileDialog::getExistingDirectory(this, "选择测试图像所在文件夹", lastDir);
    if (!dir.isEmpty())
    {
        ui->txtFolderPath->setText(dir);
        AppSettings::SetLastDirectory(AppSettings::KeyImageDir, dir);
    }
}

void BatchTestDialog::OnStartBatch()
{
    QString dirPath = ui->txtFolderPath->text().trimmed();
    if (dirPath.isEmpty() || !QDir(dirPath).exists())
    {
        QMessageBox::warning(this, "警告", "请先选择有效的图像文件夹！");
        return;
    }

    if (!_taskManager || !_taskManager->GetActiveTask())
    {
        QMessageBox::warning(this, "警告", "当前无有效检测任务可执行！");
        return;
    }

    QDir dir(dirPath);
    QStringList filters = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tiff" };
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (fileList.isEmpty())
    {
        QMessageBox::information(this, "提示", "所选文件夹内未发现支持的图片文件。");
        return;
    }

    _records.clear();
    ui->tableBatch->setRowCount(0);
    ui->progressBar->setMaximum(fileList.size());
    ui->progressBar->setValue(0);
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    _isCancelled = false;

    const DetectionTask* task = _taskManager->GetActiveTask();

    for (int i = 0; i < fileList.size(); ++i)
    {
        if (_isCancelled)
        {
            break;
        }

        const auto& fi = fileList[i];
        std::string ioErr;
        cv::Mat img = ImageIoUtils::ReadImage(fi.absoluteFilePath().toUtf8().constData(), cv::IMREAD_COLOR, ioErr);
        if (img.empty())
        {
            continue;
        }

        DetectionResult result;
        InspectionDecision decision;
        std::string errMsg;

        bool ok = DetectionExecutor::ExecuteTask(img, *task, result, decision, errMsg);

        BatchItemRecord rec;
        rec.FileName = fi.fileName();
        rec.Passed = ok && decision.IsOK();
        rec.ObjectCount = result.ObjectCount;
        rec.DefectCount = result.DefectCount;
        rec.TimeMs = result.ExecutionTimeMs;
        rec.Reason = ok ? (decision.Reasons.empty() ? "合格" : QString::fromStdString(decision.Reasons[0])) : QString::fromStdString(errMsg);

        _records.push_back(rec);

        int row = ui->tableBatch->rowCount();
        ui->tableBatch->insertRow(row);
        ui->tableBatch->setItem(row, 0, new QTableWidgetItem(rec.FileName));

        auto* statusItem = new QTableWidgetItem(rec.Passed ? "OK (合格)" : "NG (不合格)");
        statusItem->setForeground(QBrush(rec.Passed ? QColor(46, 125, 50) : QColor(198, 40, 40)));
        ui->tableBatch->setItem(row, 1, statusItem);

        ui->tableBatch->setItem(row, 2, new QTableWidgetItem(QString::number(rec.ObjectCount)));
        ui->tableBatch->setItem(row, 3, new QTableWidgetItem(QString::number(rec.DefectCount)));
        ui->tableBatch->setItem(row, 4, new QTableWidgetItem(QString::number(rec.TimeMs, 'f', 1)));
        ui->tableBatch->setItem(row, 5, new QTableWidgetItem(rec.Reason));

        ui->progressBar->setValue(i + 1);
        UpdateStatistics();
        QApplication::processEvents();
    }

    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);

    Logger::Info("BatchTestDialog", "批量检测完成，共测试 " + std::to_string(_records.size()) + " 张图像");
}

void BatchTestDialog::OnStopBatch()
{
    _isCancelled = true;
    ui->btnStop->setEnabled(false);
}

void BatchTestDialog::UpdateStatistics()
{
    int total = static_cast<int>(_records.size());
    int passed = 0;
    double totalTime = 0.0;

    for (const auto& r : _records)
    {
        if (r.Passed)
        {
            passed++;
        }
        totalTime += r.TimeMs;
    }

    int failed = total - passed;
    double passRate = (total > 0) ? (static_cast<double>(passed) / total * 100.0) : 0.0;
    double avgTime = (total > 0) ? (totalTime / total) : 0.0;

    ui->lblTotal->setText(QString("测试总数: %1").arg(total));
    ui->lblPassed->setText(QString("合格 (OK): %1").arg(passed));
    ui->lblFailed->setText(QString("不合格 (NG): %1").arg(failed));
    ui->lblPassRate->setText(QString("良品率: %1%").arg(QString::number(passRate, 'f', 1)));
    ui->lblAvgTime->setText(QString("平均耗时: %1 ms").arg(QString::number(avgTime, 'f', 1)));
}

void BatchTestDialog::OnExportCsv()
{
    if (_records.empty())
    {
        QMessageBox::information(this, "提示", "暂无批量测试记录可导出。");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, "导出测试报告 CSV", "batch_inspection_report.csv", "CSV 文件 (*.csv)");
    if (savePath.isEmpty())
    {
        return;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "错误", "无法创建 CSV 文件: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << "\xEF\xBB\xBF"; // UTF-8 BOM，防止 Excel 打开乱码
    out << "文件名,判定状态,目标数量,缺陷数量,耗时(ms),判定原因\n";

    for (const auto& r : _records)
    {
        out << "\"" << r.FileName << "\","
            << "\"" << (r.Passed ? "OK" : "NG") << "\","
            << r.ObjectCount << ","
            << r.DefectCount << ","
            << r.TimeMs << ","
            << "\"" << r.Reason << "\"\n";
    }

    file.close();
    QMessageBox::information(this, "成功", "批量检测结果已成功导出至:\n" + savePath);
}
