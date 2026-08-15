#include "PipelineListWidget.h"
#include "ui_PipelineListWidget.h"
#include <QMessageBox>

PipelineListWidget::PipelineListWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PipelineListWidget)
{
    ui->setupUi(this);

    connect(ui->btnMoveUp, &QToolButton::clicked, this, &PipelineListWidget::OnMoveUpClicked);
    connect(ui->btnMoveDown, &QToolButton::clicked, this, &PipelineListWidget::OnMoveDownClicked);
    connect(ui->btnRemove, &QToolButton::clicked, this, &PipelineListWidget::OnRemoveClicked);
    connect(ui->btnClear, &QToolButton::clicked, this, &PipelineListWidget::OnClearClicked);
    connect(ui->btnRun, &QToolButton::clicked, this, &PipelineListWidget::OnRunClicked);

    connect(ui->listSteps, &QListWidget::currentRowChanged, this, &PipelineListWidget::OnCurrentRowChanged);
    connect(ui->listSteps, &QListWidget::itemChanged, this, &PipelineListWidget::OnItemChanged);
}

PipelineListWidget::~PipelineListWidget()
{
    delete ui;
}

void PipelineListWidget::SetPipelineManager(PipelineManager* pipelineMgr)
{
    _pipelineMgr = pipelineMgr;
    if (_pipelineMgr)
    {
        connect(_pipelineMgr, &PipelineManager::PipelineChanged, this, &PipelineListWidget::RefreshList);
        connect(_pipelineMgr, &PipelineManager::ExecutionCompleted, this, [this](double totalMs) {
            ui->lblStatus->setText(QString("流水线已完成 | 步骤: %1 | 耗时: %2 ms")
                                       .arg(_pipelineMgr->GetStepCount())
                                       .arg(QString::number(totalMs, 'f', 1)));
            RefreshList();
        });
    }
    RefreshList();
}

int PipelineListWidget::GetCurrentSelectedIndex() const
{
    return ui->listSteps->currentRow();
}

void PipelineListWidget::SelectStep(int index)
{
    if (index >= 0 && index < ui->listSteps->count())
    {
        ui->listSteps->setCurrentRow(index);
    }
}

void PipelineListWidget::RefreshList()
{
    if (!_pipelineMgr)
    {
        ui->listSteps->clear();
        return;
    }

    _isUpdatingList = true;
    int savedRow = ui->listSteps->currentRow();
    ui->listSteps->clear();

    for (int i = 0; i < _pipelineMgr->GetStepCount(); ++i)
    {
        PipelineStep* step = _pipelineMgr->GetStep(i);
        if (!step || !step->GetOpNode())
        {
            continue;
        }

        QString itemText = QString("[%1] %2 (%3 ms)")
                               .arg(i + 1)
                               .arg(QString::fromStdString(step->GetOpNode()->GetDisplayName()))
                               .arg(QString::number(step->GetExecutionTimeMs(), 'f', 1));

        if (step->HasError())
        {
            itemText += " ❌ 错误";
        }

        auto* item = new QListWidgetItem(itemText, ui->listSteps);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setCheckState(step->IsEnabled() ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, i);
    }

    if (savedRow >= 0 && savedRow < ui->listSteps->count())
    {
        ui->listSteps->setCurrentRow(savedRow);
    }
    else if (ui->listSteps->count() > 0)
    {
        ui->listSteps->setCurrentRow(ui->listSteps->count() - 1);
    }

    _isUpdatingList = false;
}

void PipelineListWidget::OnMoveUpClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && row > 0)
    {
        _pipelineMgr->MoveStepUp(row);
        _pipelineMgr->Execute();
        SelectStep(row - 1);
    }
}

void PipelineListWidget::OnMoveDownClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && row >= 0 && row < _pipelineMgr->GetStepCount() - 1)
    {
        _pipelineMgr->MoveStepDown(row);
        _pipelineMgr->Execute();
        SelectStep(row + 1);
    }
}

void PipelineListWidget::OnRemoveClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && row >= 0)
    {
        _pipelineMgr->RemoveStep(row);
        _pipelineMgr->Execute();
    }
}

void PipelineListWidget::OnClearClicked()
{
    if (_pipelineMgr && _pipelineMgr->GetStepCount() > 0)
    {
        if (QMessageBox::question(this, "确认清空", "是否确定清空当前流水线中所有算子？") == QMessageBox::Yes)
        {
            _pipelineMgr->Clear();
            _pipelineMgr->Execute();
        }
    }
}

void PipelineListWidget::OnRunClicked()
{
    if (_pipelineMgr)
    {
        _pipelineMgr->Execute(true);
    }
}

void PipelineListWidget::OnCurrentRowChanged(int row)
{
    if (!_isUpdatingList)
    {
        emit StepSelected(row);
    }
}

void PipelineListWidget::OnItemChanged(QListWidgetItem* item)
{
    if (_isUpdatingList || !_pipelineMgr || !item)
    {
        return;
    }

    int row = item->data(Qt::UserRole).toInt();
    PipelineStep* step = _pipelineMgr->GetStep(row);
    if (step)
    {
        bool enabled = (item->checkState() == Qt::Checked);
        if (step->IsEnabled() != enabled)
        {
            step->SetEnabled(enabled);
            _pipelineMgr->MarkDirtyFrom(row);
            _pipelineMgr->Execute();
        }
    }
}
