#include "LogPanelWidget.h"
#include "ui_LogPanelWidget.h"
#include <QMetaObject>
#include <QScrollBar>

LogPanelWidget::LogPanelWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LogPanelWidget)
{
    ui->setupUi(this);

    connect(ui->btnClearLog, &QPushButton::clicked, this, &LogPanelWidget::ClearLog);
    connect(ui->comboLogLevel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LogPanelWidget::OnFilterChanged);

    // 订阅全局日志
    Logger::Instance().RegisterCallback([this](const LogEntry& entry) {
        QMetaObject::invokeMethod(this, [this, entry]() {
            AppendLog(entry);
        }, Qt::QueuedConnection);
    });
}

LogPanelWidget::~LogPanelWidget()
{
    delete ui;
}

void LogPanelWidget::AppendLog(const LogEntry& entry)
{
    _allEntries.push_back(entry);

    bool shouldShow = true;
    if (_currentFilterLevel == 1 && entry.Level == LogLevel::Debug)
    {
        shouldShow = false;
    }
    else if (_currentFilterLevel == 2 && (entry.Level == LogLevel::Debug || entry.Level == LogLevel::Info))
    {
        shouldShow = false;
    }
    else if (_currentFilterLevel == 3 && entry.Level != LogLevel::Error)
    {
        shouldShow = false;
    }

    if (shouldShow)
    {
        QString line = QString("[%1] [%2] [%3] %4")
            .arg(QString::fromStdString(entry.Timestamp))
            .arg(QString::fromStdString(Logger::LevelToString(entry.Level)))
            .arg(QString::fromStdString(entry.Module))
            .arg(QString::fromStdString(entry.Message));

        ui->txtLogOutput->appendPlainText(line);

        if (ui->chkAutoScroll->isChecked())
        {
            ui->txtLogOutput->verticalScrollBar()->setValue(ui->txtLogOutput->verticalScrollBar()->maximum());
        }
    }
}

void LogPanelWidget::ClearLog()
{
    _allEntries.clear();
    ui->txtLogOutput->clear();
}

void LogPanelWidget::OnFilterChanged(int index)
{
    _currentFilterLevel = index;
    RefreshDisplay();
}

void LogPanelWidget::RefreshDisplay()
{
    ui->txtLogOutput->clear();
    for (const auto& entry : _allEntries)
    {
        bool shouldShow = true;
        if (_currentFilterLevel == 1 && entry.Level == LogLevel::Debug)
        {
            shouldShow = false;
        }
        else if (_currentFilterLevel == 2 && (entry.Level == LogLevel::Debug || entry.Level == LogLevel::Info))
        {
            shouldShow = false;
        }
        else if (_currentFilterLevel == 3 && entry.Level != LogLevel::Error)
        {
            shouldShow = false;
        }

        if (shouldShow)
        {
            QString line = QString("[%1] [%2] [%3] %4")
                .arg(QString::fromStdString(entry.Timestamp))
                .arg(QString::fromStdString(Logger::LevelToString(entry.Level)))
                .arg(QString::fromStdString(entry.Module))
                .arg(QString::fromStdString(entry.Message));
            ui->txtLogOutput->appendPlainText(line);
        }
    }

    if (ui->chkAutoScroll->isChecked())
    {
        ui->txtLogOutput->verticalScrollBar()->setValue(ui->txtLogOutput->verticalScrollBar()->maximum());
    }
}
