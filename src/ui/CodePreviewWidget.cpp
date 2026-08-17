#include "CodePreviewWidget.h"
#include "ui_CodePreviewWidget.h"
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include "../utils/AppSettings.h"

CodePreviewWidget::CodePreviewWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CodePreviewWidget)
{
    ui->setupUi(this);

    connect(ui->comboLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CodePreviewWidget::OnLanguageChanged);
    connect(ui->btnCopyCode, &QPushButton::clicked, this, &CodePreviewWidget::OnCopyClicked);
    connect(ui->btnExportFile, &QPushButton::clicked, this, &CodePreviewWidget::OnExportClicked);
}

CodePreviewWidget::~CodePreviewWidget()
{
    delete ui;
}

void CodePreviewWidget::SetPipelineManager(PipelineManager* pipelineMgr)
{
    _pipelineMgr = pipelineMgr;
    if (_pipelineMgr)
    {
        connect(_pipelineMgr, &PipelineManager::PipelineChanged, this, &CodePreviewWidget::UpdateCodePreview);
        connect(_pipelineMgr, &PipelineManager::ExecutionCompleted, this, &CodePreviewWidget::UpdateCodePreview);
    }
    UpdateCodePreview();
}

void CodePreviewWidget::UpdateCodePreview()
{
    if (!_pipelineMgr)
    {
        ui->txtCode->clear();
        return;
    }

    std::string code;
    if (_currentLanguageIndex == 0)
    {
        code = _cppGen.Generate(*_pipelineMgr, "input_sample.jpg");
    }
    else
    {
        code = _pyGen.Generate(*_pipelineMgr, "input_sample.jpg");
    }

    ui->txtCode->setPlainText(QString::fromStdString(code));
}

void CodePreviewWidget::OnLanguageChanged(int index)
{
    _currentLanguageIndex = index;
    UpdateCodePreview();
}

void CodePreviewWidget::OnCopyClicked()
{
    QString code = ui->txtCode->toPlainText();
    if (code.isEmpty())
    {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(code);

    QMessageBox::information(this, "复制成功", "代码已复制到剪贴板！");
}

void CodePreviewWidget::OnExportClicked()
{
    QString code = ui->txtCode->toPlainText();
    if (code.isEmpty())
    {
        QMessageBox::warning(this, "提示", "当前没有可导出的代码！");
        return;
    }

    QString filter = (_currentLanguageIndex == 0) ? "C++ Source File (*.cpp);;All Files (*.*)" : "Python Script (*.py);;All Files (*.*)";
    QString defaultName = (_currentLanguageIndex == 0) ? "pipeline_runner.cpp" : "pipeline_runner.py";
    QString defaultPath = AppSettings::GetDefaultFilePath(AppSettings::KeyExportDir, defaultName);

    QString filePath = QFileDialog::getSaveFileName(this, "导出代码文件", defaultPath, filter);
    if (filePath.isEmpty())
    {
        return;
    }

    AppSettings::SetLastDirectory(AppSettings::KeyExportDir, filePath);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "保存失败", "无法写入文件: " + file.errorString());
        return;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << code;
    file.close();

    QMessageBox::information(this, "导出成功", "代码已成功保存至:\n" + filePath);
}
