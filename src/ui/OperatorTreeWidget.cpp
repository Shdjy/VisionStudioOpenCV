#include "OperatorTreeWidget.h"
#include "ui_OperatorTreeWidget.h"
#include "../core/OpCategory.h"
#include <map>
#include <QStyle>

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

    // 预先建立按分类分组的字典
    std::map<OpCategory, std::vector<OpMetadata>> grouped;
    for (const auto& meta : ops)
    {
        grouped[meta.Category].push_back(meta);
    }

    // 按照官方标准分类顺序插入
    auto allCats = OpCategoryUtils::GetAllCategories();
    for (auto cat : allCats)
    {
        auto it = grouped.find(cat);
        if (it == grouped.end() || it->second.empty())
        {
            continue;
        }

        auto* catItem = new QTreeWidgetItem(ui->treeOperators);
        catItem->setText(0, QString("%1  [%2]").arg(QString::fromStdString(OpCategoryUtils::ToString(cat))).arg(it->second.size()));
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
        catItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

        QFont font = catItem->font(0);
        font.setBold(true);
        catItem->setFont(0, font);

        for (const auto& meta : it->second)
        {
            auto* opItem = new QTreeWidgetItem(catItem);
            opItem->setText(0, QString("%1").arg(QString::fromStdString(meta.DisplayName)));
            opItem->setToolTip(0, QString::fromStdString(meta.Description));
            opItem->setData(0, Qt::UserRole, QString::fromStdString(meta.Name));
            opItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }
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
