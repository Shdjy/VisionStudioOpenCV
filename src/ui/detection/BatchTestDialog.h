#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include "../../detection/DetectionTaskManager.h"

namespace Ui {
class BatchTestDialog;
}

/**
 * @brief 批量检测与离线评估对话框
 */
class BatchTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchTestDialog(
        DetectionTaskManager* taskManager,
        QWidget* parent = nullptr);
    ~BatchTestDialog() override;

private slots:
    void OnBrowseFolder();
    void OnStartBatch();
    void OnStopBatch();
    void OnExportCsv();

private:
    Ui::BatchTestDialog* ui;
    DetectionTaskManager* _taskManager = nullptr;
    bool _isCancelled = false;

    struct BatchItemRecord
    {
        QString FileName;
        bool Passed = false;
        int ObjectCount = 0;
        int DefectCount = 0;
        double TimeMs = 0.0;
        QString Reason;
    };

    std::vector<BatchItemRecord> _records;

    void UpdateStatistics();
};
