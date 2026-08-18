#pragma once

#include <QWidget>
#include <QListWidgetItem>
#include "../bll/PipelineManager.h"

namespace Ui {
class PipelineListWidget;
}

class PipelineListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PipelineListWidget(QWidget* parent = nullptr);
    ~PipelineListWidget() override;

    void SetPipelineManager(PipelineManager* pipelineMgr);
    int GetCurrentSelectedIndex() const;
    void SelectStep(int index);

signals:
    void StepSelected(int index);
    void StepDoubleClicked(int index);

public slots:
    void RefreshList();
    void OnClearClicked();

private slots:
    void OnMoveUpClicked();
    void OnMoveDownClicked();
    void OnRemoveClicked();
    void OnSelectAllClicked();
    void OnDeselectAllClicked();
    void OnRunClicked();
    void OnCurrentRowChanged(int row);
    void OnItemChanged(QListWidgetItem* item);

private:
    Ui::PipelineListWidget* ui;
    PipelineManager* _pipelineMgr = nullptr;
    bool _isUpdatingList = false;
};
