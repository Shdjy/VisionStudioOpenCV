#include "PipelineListWidget.h"
#include "ui_PipelineListWidget.h"
#include "../utils/IconUtils.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QStyle>

PipelineListWidget::PipelineListWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PipelineListWidget)
{
    ui->setupUi(this);

    // 设置与顶部主菜单一致的紧凑图标栏风格
    const QSize btnIconSize(18, 18);
    const QSize btnSize(28, 28);

    ui->btnMoveUp->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    ui->btnMoveUp->setIconSize(btnIconSize);
    ui->btnMoveUp->setFixedSize(btnSize);
    ui->btnMoveUp->setToolTip("上移所选算子步骤 (Up)");

    ui->btnMoveDown->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    ui->btnMoveDown->setIconSize(btnIconSize);
    ui->btnMoveDown->setFixedSize(btnSize);
    ui->btnMoveDown->setToolTip("下移所选算子步骤 (Down)");

    ui->btnRemove->setIcon(IconUtils::CreateRedCrossIcon(32));
    ui->btnRemove->setIconSize(btnIconSize);
    ui->btnRemove->setFixedSize(btnSize);
    ui->btnRemove->setToolTip("删除所选算子步骤 (Delete / 红色❌)");

    ui->btnToggleAll->setIcon(IconUtils::CreateSelectAllIcon(QColor(76, 175, 80), 32));
    ui->btnToggleAll->setIconSize(btnIconSize);
    ui->btnToggleAll->setFixedSize(btnSize);
    ui->btnToggleAll->setToolTip("一键全选 / 取消全选切换 (Toggle Select All)");

    ui->btnCreateBranch->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->btnCreateBranch->setIconSize(btnIconSize);
    ui->btnCreateBranch->setFixedSize(btnSize);
    ui->btnCreateBranch->setToolTip("基于当前最新图像创建新分支 (🌿 Fork Branch)");

    ui->btnClear->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    ui->btnClear->setIconSize(btnIconSize);
    ui->btnClear->setFixedSize(btnSize);
    ui->btnClear->setToolTip("清空当前流水线所有步骤 (Clear)");

    ui->btnRun->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->btnRun->setIconSize(btnIconSize);
    ui->btnRun->setFixedSize(btnSize);
    ui->btnRun->setToolTip("立即执行流水线 (Run / F5)");

    connect(ui->btnMoveUp, &QToolButton::clicked, this, &PipelineListWidget::OnMoveUpClicked);
    connect(ui->btnMoveDown, &QToolButton::clicked, this, &PipelineListWidget::OnMoveDownClicked);
    connect(ui->btnRemove, &QToolButton::clicked, this, &PipelineListWidget::OnRemoveClicked);
    connect(ui->btnToggleAll, &QToolButton::clicked, this, &PipelineListWidget::OnToggleAllClicked);
    connect(ui->btnCreateBranch, &QToolButton::clicked, this, &PipelineListWidget::OnCreateBranchClicked);
    connect(ui->btnClear, &QToolButton::clicked, this, &PipelineListWidget::OnClearClicked);
    connect(ui->btnRun, &QToolButton::clicked, this, &PipelineListWidget::OnRunClicked);

    connect(ui->listSteps, &QListWidget::currentRowChanged, this, &PipelineListWidget::OnCurrentRowChanged);
    connect(ui->listSteps, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (!_isUpdatingList && item)
        {
            emit StepSelected(ui->listSteps->row(item));
        }
    });
    connect(ui->listSteps, &QListWidget::itemPressed, this, [this](QListWidgetItem* item) {
        if (!_isUpdatingList && item)
        {
            emit StepSelected(ui->listSteps->row(item));
        }
    });
    connect(ui->listSteps, &QListWidget::itemChanged, this, &PipelineListWidget::OnItemChanged);

    // 分支 Tab 栏信号连接
    connect(ui->tabBranches, &QTabBar::currentChanged, this, &PipelineListWidget::OnBranchTabChanged);
    connect(ui->tabBranches, &QTabBar::tabCloseRequested, this, &PipelineListWidget::OnBranchTabCloseRequested);
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
        connect(_pipelineMgr, &PipelineManager::BranchListChanged, this, &PipelineListWidget::RefreshBranchTabs);
        connect(_pipelineMgr, &PipelineManager::ActiveBranchChanged, this, [this](int idx) {
            _selectLastStepOnRefresh = true;
            RefreshBranchTabs();
            RefreshList();
            emit BranchSelected(idx);
        });
        connect(_pipelineMgr, &PipelineManager::ExecutionCompleted, this, [this](double totalMs) {
            std::string branchName = _pipelineMgr->GetBranchName(_pipelineMgr->GetActiveBranchIndex());
            ui->lblStatus->setText(QString("[%1] 已完成 | 步骤: %2 | 耗时: %3 ms")
                                       .arg(QString::fromStdString(branchName))
                                       .arg(_pipelineMgr->GetStepCount())
                                       .arg(QString::number(totalMs, 'f', 1)));
            RefreshList();
        });
    }
    _selectLastStepOnRefresh = true;
    RefreshBranchTabs();
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
        emit StepSelected(index);
    }
    else if (index == -1)
    {
        ui->listSteps->setCurrentRow(-1);
        emit StepSelected(-1);
    }
}

void PipelineListWidget::RefreshBranchTabs()
{
    if (!_pipelineMgr)
    {
        while (ui->tabBranches->count() > 0)
        {
            ui->tabBranches->removeTab(0);
        }
        return;
    }

    _isUpdatingTabs = true;
    while (ui->tabBranches->count() > 0)
    {
        ui->tabBranches->removeTab(0);
    }

    int count = _pipelineMgr->GetBranchCount();
    for (int i = 0; i < count; ++i)
    {
        QString tabText = QString::fromStdString(_pipelineMgr->GetBranchName(i));
        int tabIdx = ui->tabBranches->addTab(tabText);

        // 主流水线 (Tab 0) 不允许关闭
        if (i == 0)
        {
            ui->tabBranches->setTabButton(tabIdx, QTabBar::RightSide, nullptr);
        }
    }

    int activeIdx = _pipelineMgr->GetActiveBranchIndex();
    if (activeIdx >= 0 && activeIdx < ui->tabBranches->count())
    {
        ui->tabBranches->setCurrentIndex(activeIdx);
    }

    _isUpdatingTabs = false;
}

void PipelineListWidget::RefreshList()
{
    if (!_pipelineMgr)
    {
        ui->listSteps->clear();
        emit StepSelected(-1);
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

        // 步骤状态色彩提示 (错误时醒目红，禁用时淡灰，正常时清晰白)
        if (step->HasError())
        {
            item->setForeground(QBrush(QColor(255, 107, 107)));
        }
        else if (!step->IsEnabled())
        {
            item->setForeground(QBrush(QColor(140, 140, 140)));
        }
        else
        {
            item->setForeground(QBrush(QColor(230, 230, 230)));
        }
    }

    int targetRow = -1;
    if (_selectLastStepOnRefresh)
    {
        targetRow = ui->listSteps->count() - 1;
        _selectLastStepOnRefresh = false;
    }
    else if (savedRow >= 0 && savedRow < ui->listSteps->count())
    {
        targetRow = savedRow;
    }
    else if (ui->listSteps->count() > 0)
    {
        targetRow = ui->listSteps->count() - 1;
    }

    ui->listSteps->setCurrentRow(targetRow);
    _isUpdatingList = false;

    UpdateToggleAllButtonState();

    if (targetRow >= 0)
    {
        emit StepSelected(targetRow);
    }
    else
    {
        emit StepSelected(-1);
    }
}

void PipelineListWidget::UpdateToggleAllButtonState()
{
    if (!_pipelineMgr || _pipelineMgr->GetStepCount() == 0)
    {
        ui->btnToggleAll->setChecked(false);
        ui->btnToggleAll->setIcon(IconUtils::CreateSelectAllIcon(QColor(76, 175, 80), 32));
        ui->btnToggleAll->setToolTip("一键全选当前分支所有流程");
        return;
    }

    bool allEnabled = _pipelineMgr->AreAllStepsEnabled();
    ui->btnToggleAll->setChecked(allEnabled);
    if (allEnabled)
    {
        ui->btnToggleAll->setIcon(IconUtils::CreateDeselectAllIcon(QColor(230, 80, 80), 32));
        ui->btnToggleAll->setToolTip("一键取消/禁用当前分支所有流程 (当前已全部启用)");
    }
    else
    {
        ui->btnToggleAll->setIcon(IconUtils::CreateSelectAllIcon(QColor(76, 175, 80), 32));
        ui->btnToggleAll->setToolTip("一键全选/启用当前分支所有流程");
    }
}

void PipelineListWidget::OnMoveUpClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && _pipelineMgr->MoveStepUp(row))
    {
        _pipelineMgr->Execute();
        ui->listSteps->setCurrentRow(row - 1);
    }
}

void PipelineListWidget::OnMoveDownClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && _pipelineMgr->MoveStepDown(row))
    {
        _pipelineMgr->Execute();
        ui->listSteps->setCurrentRow(row + 1);
    }
}

void PipelineListWidget::OnRemoveClicked()
{
    int row = ui->listSteps->currentRow();
    if (_pipelineMgr && _pipelineMgr->RemoveStep(row))
    {
        _pipelineMgr->Execute();
        int newRow = std::min(row, ui->listSteps->count() - 1);
        ui->listSteps->setCurrentRow(newRow);
    }
}

void PipelineListWidget::OnToggleAllClicked()
{
    if (!_pipelineMgr || _pipelineMgr->GetStepCount() == 0)
    {
        return;
    }

    bool allEnabled = _pipelineMgr->AreAllStepsEnabled();
    // 若当前全部启用，则一键禁用；反之则一键全部启用
    _pipelineMgr->SetAllStepsEnabled(!allEnabled);
    UpdateToggleAllButtonState();
}

void PipelineListWidget::OnCreateBranchClicked()
{
    if (!_pipelineMgr)
    {
        return;
    }

    int newIdx = _pipelineMgr->CreateBranch();
    RefreshBranchTabs();
    emit BranchSelected(newIdx);
    _pipelineMgr->Execute();
}

void PipelineListWidget::OnClearClicked()
{
    if (!_pipelineMgr || _pipelineMgr->GetStepCount() == 0)
    {
        return;
    }

    auto reply = QMessageBox::question(this, "清空确认", "确定要清空当前流水线分支的所有算子步骤吗？",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        _pipelineMgr->Clear();
        _pipelineMgr->Execute();
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
            UpdateToggleAllButtonState();
        }
    }
}

void PipelineListWidget::OnBranchTabChanged(int index)
{
    if (_isUpdatingTabs || !_pipelineMgr || index < 0 || index >= _pipelineMgr->GetBranchCount())
    {
        return;
    }

    _selectLastStepOnRefresh = true;
    _pipelineMgr->SetActiveBranchIndex(index);
    emit BranchSelected(index);
    _pipelineMgr->Execute();
}

void PipelineListWidget::OnBranchTabCloseRequested(int index)
{
    if (!_pipelineMgr || index <= 0)
    {
        return; // 主流水线不可删除
    }

    std::string bName = _pipelineMgr->GetBranchName(index);
    auto reply = QMessageBox::question(this, "删除分支确认",
                                       QString("确定要删除分支【%1】及其所有算子流程吗？").arg(QString::fromStdString(bName)),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        _pipelineMgr->RemoveBranch(index);
        RefreshBranchTabs();
        RefreshList();
    }
}
