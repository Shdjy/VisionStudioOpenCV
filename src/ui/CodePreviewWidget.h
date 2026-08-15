#pragma once

#include <QWidget>
#include "../bll/PipelineManager.h"
#include "../codegen/CppCodeGenerator.h"
#include "../codegen/PythonCodeGenerator.h"

namespace Ui {
class CodePreviewWidget;
}

class CodePreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CodePreviewWidget(QWidget* parent = nullptr);
    ~CodePreviewWidget() override;

    void SetPipelineManager(PipelineManager* pipelineMgr);

    QSize sizeHint() const override { return QSize(700, 220); }
    QSize minimumSizeHint() const override { return QSize(300, 140); }

public slots:
    void UpdateCodePreview();

private slots:
    void OnLanguageChanged(int index);
    void OnCopyClicked();
    void OnExportClicked();

private:
    Ui::CodePreviewWidget* ui;
    PipelineManager* _pipelineMgr = nullptr;
    CppCodeGenerator _cppGen;
    PythonCodeGenerator _pyGen;
    int _currentLanguageIndex = 0; // 0: C++, 1: Python
};
