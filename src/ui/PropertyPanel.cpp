#include "PropertyPanel.h"
#include "ui_PropertyPanel.h"
#include "../core/OpRegistry.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include "../utils/AppSettings.h"
#include <QHBoxLayout>
#include <QEvent>

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PropertyPanel)
{
    ui->setupUi(this);

    _executionDebounceTimer = new QTimer(this);
    _executionDebounceTimer->setSingleShot(true);
    _executionDebounceTimer->setInterval(150);
    connect(_executionDebounceTimer, &QTimer::timeout, this, [this]() {
        if (_pipelineMgr && _pendingStepIndex >= 0)
        {
            const int stepIndex = _pendingStepIndex;
            _pendingStepIndex = -1;
            _pipelineMgr->MarkDirtyFrom(stepIndex);
            _pipelineMgr->Execute();
            emit ParameterChanged(stepIndex);
        }
    });
}

PropertyPanel::~PropertyPanel()
{
    delete ui;
}

bool PropertyPanel::eventFilter(QObject* watched, QEvent* event)
{
    // 拦截参数调节控件上的滚轮事件，防止滚动页面时误触；
    // 但若微调输入框已获得焦点，则允许通过滚轮顺畅微调
    if (event->type() == QEvent::Wheel)
    {
        auto* spin = qobject_cast<QAbstractSpinBox*>(watched);
        if (spin && spin->hasFocus())
        {
            return false;
        }
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void PropertyPanel::SetPipelineManager(PipelineManager* pipelineMgr)
{
    _pipelineMgr = pipelineMgr;
}

void PropertyPanel::ScheduleExecution(int stepIndex)
{
    _pendingStepIndex = stepIndex;
    _executionDebounceTimer->start();
}

void PropertyPanel::SetCurrentStep(int stepIndex)
{
    _isPreviewMode = false;
    _previewOpNode.reset();
    _currentStepIndex = stepIndex;

    if (!_pipelineMgr)
    {
        ClearDynamicWidgets();
        ui->lblOpTitle->setText("未连接流水线");
        ui->lblOpDesc->setText("");
        ui->textDoc->clear();
        return;
    }

    PipelineStep* step = _pipelineMgr->GetStep(stepIndex);
    if (!step || !step->GetOpNode())
    {
        ClearDynamicWidgets();
        ui->lblOpTitle->setText("未选中算子");
        ui->lblOpDesc->setText("请在左侧流水线列表中选择一个算子，或在算子树中点击查看原理。");
        ui->textDoc->clear();
        return;
    }

    IOpNode* op = step->GetOpNode();
    ui->lblOpTitle->setText(QString::fromStdString(op->GetDisplayName()));
    ui->lblOpDesc->setText(QString::fromStdString(op->GetDescription()));
    ui->textDoc->setMarkdown(QString::fromStdString(op->GetPrincipleDoc()));

    BuildParamControls(op, false);
}

void PropertyPanel::ShowOperatorPreview(const QString& opName)
{
    if (opName.isEmpty())
    {
        return;
    }

    auto op = OpRegistry::Instance().Create(opName.toStdString());
    if (!op)
    {
        return;
    }

    _isPreviewMode = true;
    _currentStepIndex = -1;
    _previewOpNode = std::move(op);

    ui->lblOpTitle->setText(QString("[教学预览] %1").arg(QString::fromStdString(_previewOpNode->GetDisplayName())));
    ui->lblOpDesc->setText(QString("%1\n(💡 提示：双击或点击添加按钮可将此算子加入流水线)").arg(QString::fromStdString(_previewOpNode->GetDescription())));
    ui->textDoc->setMarkdown(QString::fromStdString(_previewOpNode->GetPrincipleDoc()));

    BuildParamControls(_previewOpNode.get(), true);
}

void PropertyPanel::ClearDynamicWidgets()
{
    QLayoutItem* child;
    while ((child = ui->dynamicParamsLayout->takeAt(0)) != nullptr)
    {
        if (child->widget())
        {
            child->widget()->deleteLater();
        }
        if (child->layout())
        {
            delete child->layout();
        }
        delete child;
    }
}

void PropertyPanel::BuildParamControls(IOpNode* opNode, bool isPreview)
{
    ClearDynamicWidgets();

    if (!opNode)
    {
        return;
    }

    if (isPreview)
    {
        auto* tipLabel = new QLabel("📌 默认参数配置展示 (加入流水线后可实时调参):", this);
        tipLabel->setStyleSheet("color: #f39c12; font-weight: bold; padding: 4px; background: #2d2d30; border-radius: 3px;");
        ui->dynamicParamsLayout->addWidget(tipLabel);
    }

    auto& params = opNode->GetParameters();
    for (size_t i = 0; i < params.size(); ++i)
    {
        OpParameter& param = params[i];
        std::string paramName = param.Name;

        auto* rowWidget = new QWidget(this);
        auto* rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(2, 2, 2, 2);

        auto* nameLabel = new QLabel(QString::fromStdString(param.DisplayName), rowWidget);
        nameLabel->setToolTip(QString::fromStdString(param.Tooltip));
        nameLabel->setStyleSheet("font-weight: bold; color: #5dade2;");
        rowLayout->addWidget(nameLabel);

        if (param.Type == ParamType::IntSlider)
        {
            auto* ctrlLayout = new QHBoxLayout();
            auto* slider = new QSlider(Qt::Horizontal, rowWidget);
            auto* spinBox = new QSpinBox(rowWidget);

            slider->installEventFilter(this);
            spinBox->installEventFilter(this);
            slider->setFocusPolicy(Qt::NoFocus);
            spinBox->setFocusPolicy(Qt::ClickFocus);

            int minVal = std::holds_alternative<int>(param.MinValue) ? std::get<int>(param.MinValue) : 0;
            int maxVal = std::holds_alternative<int>(param.MaxValue) ? std::get<int>(param.MaxValue) : 100;
            int step = std::holds_alternative<int>(param.Step) ? std::get<int>(param.Step) : 1;
            if (step <= 0)
            {
                step = 1;
            }
            int curVal = param.GetInt();

            spinBox->setMinimumWidth(85);
            spinBox->setAlignment(Qt::AlignRight);
            spinBox->setRange(minVal, maxVal);
            spinBox->setSingleStep(step);
            spinBox->setValue(curVal);

            slider->setRange(minVal, maxVal);
            slider->setSingleStep(step);
            slider->setPageStep(std::max(step * 5, 10));
            slider->setValue(curVal);

            slider->setEnabled(!isPreview);
            spinBox->setEnabled(!isPreview);

            if (!isPreview)
            {
                connect(slider, &QSlider::valueChanged, spinBox, &QSpinBox::setValue);
                connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

                connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, paramName](int val) {
                    if (_pipelineMgr && _currentStepIndex >= 0)
                    {
                        PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                        if (s && s->GetOpNode())
                        {
                            s->GetOpNode()->SetParameter(paramName, val);
                            ScheduleExecution(_currentStepIndex);
                        }
                    }
                });
            }

            ctrlLayout->addWidget(slider, 1);
            ctrlLayout->addWidget(spinBox);
            rowLayout->addLayout(ctrlLayout);
        }
        else if (param.Type == ParamType::DoubleSlider)
        {
            auto* ctrlLayout = new QHBoxLayout();
            auto* slider = new QSlider(Qt::Horizontal, rowWidget);
            auto* spinBox = new QDoubleSpinBox(rowWidget);

            slider->installEventFilter(this);
            spinBox->installEventFilter(this);
            slider->setFocusPolicy(Qt::NoFocus);
            spinBox->setFocusPolicy(Qt::ClickFocus);

            double minVal = param.MinValue.index() == 0 ? std::get<int>(param.MinValue) : std::get<double>(param.MinValue);
            double maxVal = param.MaxValue.index() == 0 ? std::get<int>(param.MaxValue) : std::get<double>(param.MaxValue);
            double step = param.Step.index() == 0 ? std::get<int>(param.Step) : std::get<double>(param.Step);
            if (step <= 0.0)
            {
                step = (maxVal - minVal >= 100.0) ? 1.0 : ((maxVal - minVal >= 10.0) ? 0.1 : 0.01);
            }
            double curVal = param.GetDouble();

            int decimals = 2;
            if (step < 0.01)
            {
                decimals = 3;
            }
            else if (step >= 1.0 && maxVal >= 100.0)
            {
                decimals = 1;
            }

            int scale = (decimals >= 3) ? 1000 : ((decimals == 2) ? 100 : 10);
            if ((maxVal - minVal) * scale > 1000000.0)
            {
                scale = static_cast<int>(1000000.0 / (maxVal - minVal));
                if (scale < 1)
                {
                    scale = 1;
                }
            }

            spinBox->setMinimumWidth(85);
            spinBox->setAlignment(Qt::AlignRight);
            spinBox->setRange(minVal, maxVal);
            spinBox->setSingleStep(step);
            spinBox->setDecimals(decimals);
            spinBox->setValue(curVal);

            slider->setRange(static_cast<int>(minVal * scale), static_cast<int>(maxVal * scale));
            slider->setSingleStep(std::max(1, static_cast<int>(step * scale)));
            slider->setPageStep(std::max(5, static_cast<int>(step * scale * 5)));
            slider->setValue(static_cast<int>(curVal * scale));

            slider->setEnabled(!isPreview);
            spinBox->setEnabled(!isPreview);

            if (!isPreview)
            {
                connect(slider, &QSlider::valueChanged, [spinBox, scale](int val) {
                    spinBox->setValue(val / static_cast<double>(scale));
                });

                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [slider, scale](double val) {
                    slider->blockSignals(true);
                    slider->setValue(static_cast<int>(val * scale));
                    slider->blockSignals(false);
                });

                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, paramName](double val) {
                    if (_pipelineMgr && _currentStepIndex >= 0)
                    {
                        PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                        if (s && s->GetOpNode())
                        {
                            s->GetOpNode()->SetParameter(paramName, val);
                            ScheduleExecution(_currentStepIndex);
                        }
                    }
                });
            }

            ctrlLayout->addWidget(slider, 1);
            ctrlLayout->addWidget(spinBox);
            rowLayout->addLayout(ctrlLayout);
        }
        else if (param.Type == ParamType::ComboBox)
        {
            auto* combo = new QComboBox(rowWidget);
            combo->installEventFilter(this);
            combo->setFocusPolicy(Qt::StrongFocus);

            for (const auto& opt : param.EnumOptions)
            {
                combo->addItem(QString::fromStdString(opt));
            }
            combo->setCurrentIndex(param.GetInt());
            combo->setEnabled(!isPreview);

            if (!isPreview)
            {
                connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, paramName](int idx) {
                    if (_pipelineMgr && _currentStepIndex >= 0)
                    {
                        PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                        if (s && s->GetOpNode())
                        {
                            s->GetOpNode()->SetParameter(paramName, idx);
                            ScheduleExecution(_currentStepIndex);
                        }
                    }
                });
            }

            rowLayout->addWidget(combo);
        }
        else if (param.Type == ParamType::CheckBox)
        {
            auto* chk = new QCheckBox(QString::fromStdString(param.DisplayName), rowWidget);
            chk->setChecked(param.GetBool());
            chk->setEnabled(!isPreview);

            if (!isPreview)
            {
                connect(chk, &QCheckBox::toggled, [this, paramName](bool checked) {
                    if (_pipelineMgr && _currentStepIndex >= 0)
                    {
                        PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                        if (s && s->GetOpNode())
                        {
                            s->GetOpNode()->SetParameter(paramName, checked);
                            ScheduleExecution(_currentStepIndex);
                        }
                    }
                });
            }

            rowLayout->addWidget(chk);
        }
        else if (param.Type == ParamType::FilePath)
        {
            auto* fileLayout = new QHBoxLayout();
            auto* lineEdit = new QLineEdit(QString::fromStdString(param.GetString()), rowWidget);
            lineEdit->setPlaceholderText("请选择本地图像文件路径...");
            lineEdit->setReadOnly(true);
            lineEdit->setEnabled(!isPreview);

            auto* btnBrowse = new QPushButton("📁 浏览...", rowWidget);
            btnBrowse->setCursor(Qt::PointingHandCursor);
            btnBrowse->setEnabled(!isPreview);

            if (!isPreview)
            {
                connect(btnBrowse, &QPushButton::clicked, [this, lineEdit, paramName]() {
                    QString initialDir = AppSettings::GetLastDirectory(AppSettings::KeyTemplateDir);
                    QString filePath = QFileDialog::getOpenFileName(
                        this,
                        "选择本地模板图像文件",
                        initialDir,
                        "图像文件 (*.png *.jpg *.jpeg *.bmp *.tif *.webp);;所有文件 (*.*)"
                    );
                    if (!filePath.isEmpty())
                    {
                        AppSettings::SetLastDirectory(AppSettings::KeyTemplateDir, filePath);
                        lineEdit->setText(filePath);
                        if (_pipelineMgr && _currentStepIndex >= 0)
                        {
                            PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                            if (s && s->GetOpNode())
                            {
                                s->GetOpNode()->SetParameter(paramName, filePath.toStdString());
                                ScheduleExecution(_currentStepIndex);
                            }
                        }
                    }
                });
            }

            fileLayout->addWidget(lineEdit, 1);
            fileLayout->addWidget(btnBrowse);
            rowLayout->addLayout(fileLayout);
        }

        ui->dynamicParamsLayout->addWidget(rowWidget);
    }

    ui->dynamicParamsLayout->addStretch();
}
