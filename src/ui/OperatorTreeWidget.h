#pragma once

#include <QWidget>
#include <QTreeWidgetItem>
#include "../core/OpRegistry.h"

namespace Ui {
class OperatorTreeWidget;
}

class OperatorTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorTreeWidget(QWidget* parent = nullptr);
    ~OperatorTreeWidget() override;

    void PopulateOperators();

signals:
    void OperatorSelected(const QString& opName);
    void OperatorAddRequested(const QString& opName);

private slots:
    void OnSearchTextChanged(const QString& text);
    void OnItemDoubleClicked(QTreeWidgetItem* item, int column);
    void OnAddButtonClicked();

private:
    Ui::OperatorTreeWidget* ui;
    void BuildCategoryTree(const std::vector<OpMetadata>& ops);
};
