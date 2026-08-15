#include "OperatorTreeWidget.h"
#include "ui_OperatorTreeWidget.h"
#include <map>

OperatorTreeWidget::OperatorTreeWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OperatorTreeWidget)
{
    ui->setupUi(this);

    connect(ui->editSearch, &QLineEdit::textChanged, this, &OperatorTreeWidget::OnSearchTextChanged);
    connect(ui->treeOperators, &QTreeWidget::itemDoubleClicked, this, &OperatorTreeWidget::OnItemDoubleClicked);
    connect(ui->btnAddOperator, &QPushButton::clicked, this, &OperatorTreeWidget::OnAddButtonClicked);

    PopulateOperators();
}

OperatorTreeWidget::~OperatorTreeWidget()
{
    delete ui;
}

void OperatorTreeWidget::PopulateOperators()
{
    auto allOps = OpRegistry::Instance().GetAllOperators();
    BuildCategoryTree(allOps);
}

void OperatorTreeWidget::BuildCategoryTree(const std::vector<OpMetadata>& ops)
{
    ui->treeOperators->clear();

    std::map<OpCategory, QTreeWidgetItem*> categoryNodes;

    for (OpCategory cat : OpCategoryUtils::GetAllCategories())
    {
        auto* catItem = new QTreeWidgetItem(ui->treeOperators);
        catItem->setText(0, QString::fromStdString(OpCategoryUtils::ToString(cat)));
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
        categoryNodes[cat] = catItem;
    }

    for (const auto& meta : ops)
    {
        auto it = categoryNodes.find(meta.Category);
        if (it != categoryNodes.end())
        {
            auto* opItem = new QTreeWidgetItem(it->second);
            opItem->setText(0, QString::fromStdString(meta.DisplayName));
            opItem->setToolTip(0, QString::fromStdString(meta.Description));
            opItem->setData(0, Qt::UserRole, QString::fromStdString(meta.Name));
        }
    }

    // 移除没有子项的空分类并展开包含项的分类
    for (auto& pair : categoryNodes)
    {
        if (pair.second->childCount() == 0)
        {
            delete pair.second;
        }
        else
        {
            pair.second->setExpanded(true);
        }
    }
}

void OperatorTreeWidget::OnSearchTextChanged(const QString& text)
{
    auto filtered = OpRegistry::Instance().SearchOperators(text.toStdString());
    BuildCategoryTree(filtered);
}

void OperatorTreeWidget::OnItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item || item->childCount() > 0)
    {
        return; // 双击的是分类父节点
    }

    QString opName = item->data(0, Qt::UserRole).toString();
    if (!opName.isEmpty())
    {
        emit OperatorAddRequested(opName);
    }
}

void OperatorTreeWidget::OnAddButtonClicked()
{
    QTreeWidgetItem* item = ui->treeOperators->currentItem();
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
