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
    void BranchSelected(int branchIndex);

public slots:
    void RefreshList();
    void RefreshBranchTabs();
    void OnClearClicked();
    void OnToggleAllClicked();
    void OnCreateBranchClicked();

private slots:
    void OnMoveUpClicked();
    void OnMoveDownClicked();
    void OnRemoveClicked();
    void OnRunClicked();
    void OnCurrentRowChanged(int row);
    void OnItemChanged(QListWidgetItem* item);
    void OnBranchTabChanged(int index);
    void OnBranchTabCloseRequested(int index);

private:
    Ui::PipelineListWidget* ui;
    PipelineManager* _pipelineMgr = nullptr;
    bool _isUpdatingList = false;
    bool _isUpdatingTabs = false;
    bool _selectLastStepOnRefresh = false;
    void UpdateToggleAllButtonState();
};

