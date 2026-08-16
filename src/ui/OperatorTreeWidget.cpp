#include "OperatorTreeWidget.h"
#include "ui_OperatorTreeWidget.h"
#include "../core/OpCategory.h"
#include <map>

OperatorTreeWidget::OperatorTreeWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OperatorTreeWidget)
{
    ui->setupUi(this);

    connect(ui->editSearch, &QLineEdit::textChanged, this, &OperatorTreeWidget::OnSearchTextChanged);
    connect(ui->treeOperators, &QTreeWidget::itemDoubleClicked, this, &OperatorTreeWidget::OnItemDoubleClicked);
    connect(ui->treeOperators, &QTreeWidget::itemClicked, [this](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        if (item && item->childCount() == 0)
        {
            QString opName = item->data(0, Qt::UserRole).toString();
            if (!opName.isEmpty())
            {
                emit OperatorSelected(opName);
            }
        }
    });
    connect(ui->treeOperators, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
        Q_UNUSED(previous);
        if (current && current->childCount() == 0)
        {
            QString opName = current->data(0, Qt::UserRole).toString();
            if (!opName.isEmpty())
            {
                emit OperatorSelected(opName);
            }
        }
    });

    connect(ui->btnAddOperator, &QPushButton::clicked, this, &OperatorTreeWidget::OnAddButtonClicked);
    connect(ui->btnExpandAll, &QToolButton::clicked, ui->treeOperators, &QTreeWidget::expandAll);
    connect(ui->btnCollapseAll, &QToolButton::clicked, ui->treeOperators, &QTreeWidget::collapseAll);

    PopulateOperators();
}

OperatorTreeWidget::~OperatorTreeWidget()
{
    delete ui;
}

void OperatorTreeWidget::PopulateOperators()
{
    auto ops = OpRegistry::Instance().GetAllOperators();
    BuildCategoryTree(ops);
}

void OperatorTreeWidget::BuildCategoryTree(const std::vector<OpMetadata>& ops)
{
    ui->treeOperators->clear();

    std::map<OpCategory, QTreeWidgetItem*> catItems;

    for (const auto& meta : ops)
    {
        if (catItems.find(meta.Category) == catItems.end())
        {
            auto* catItem = new QTreeWidgetItem(ui->treeOperators);
            catItem->setText(0, QString::fromStdString(OpCategoryUtils::ToString(meta.Category)));
            catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
            catItem->setIcon(0, QIcon());
            catItems[meta.Category] = catItem;
        }

        auto* opItem = new QTreeWidgetItem(catItems[meta.Category]);
        opItem->setText(0, QString("%1 (%2)").arg(QString::fromStdString(meta.DisplayName), QString::fromStdString(meta.Name)));
        opItem->setToolTip(0, QString::fromStdString(meta.Description));
        opItem->setData(0, Qt::UserRole, QString::fromStdString(meta.Name));
    }

    ui->treeOperators->expandAll();
}

void OperatorTreeWidget::OnSearchTextChanged(const QString& text)
{
    if (text.trimmed().isEmpty())
    {
        PopulateOperators();
        return;
    }

    auto matchedOps = OpRegistry::Instance().SearchOperators(text.toStdString());
    BuildCategoryTree(matchedOps);
    ui->treeOperators->expandAll();
}

void OperatorTreeWidget::OnItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item || item->childCount() > 0)
    {
        return;
    }

    QString opName = item->data(0, Qt::UserRole).toString();
    if (!opName.isEmpty())
    {
        emit OperatorAddRequested(opName);
    }
}

void OperatorTreeWidget::OnAddButtonClicked()
{
    QTreeWidgetItem* cur = ui->treeOperators->currentItem();
    if (cur && cur->childCount() == 0)
    {
        QString opName = cur->data(0, Qt::UserRole).toString();
        if (!opName.isEmpty())
        {
            emit OperatorAddRequested(opName);
        }
    }
}
