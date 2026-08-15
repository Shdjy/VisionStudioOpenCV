#pragma once

#include <QWidget>
#include "../utils/Logger.h"

namespace Ui {
class LogPanelWidget;
}

class LogPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogPanelWidget(QWidget* parent = nullptr);
    ~LogPanelWidget() override;

public slots:
    void AppendLog(const LogEntry& entry);
    void ClearLog();

private slots:
    void OnFilterChanged(int index);

private:
    Ui::LogPanelWidget* ui;
    int _currentFilterLevel = 0; // 0: All, 1: Info+, 2: Warn+, 3: Error
    std::vector<LogEntry> _allEntries;

    void RefreshDisplay();
};
