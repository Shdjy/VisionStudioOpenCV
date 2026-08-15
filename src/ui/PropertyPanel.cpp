#include "PropertyPanel.h"
#include "ui_PropertyPanel.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PropertyPanel)
{
    ui->setupUi(this);
}

PropertyPanel::~PropertyPanel()
{
    delete ui;
}

void PropertyPanel::SetPipelineManager(PipelineManager* pipelineMgr)
{
    _pipelineMgr = pipelineMgr;
}

void PropertyPanel::SetCurrentStep(int stepIndex)
{
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
        ui->lblOpDesc->setText("请在流水线列表中选择一个算子进行参数调节。");
        ui->textDoc->clear();
        return;
    }

    IOpNode* op = step->GetOpNode();
    ui->lblOpTitle->setText(QString::fromStdString(op->GetDisplayName()));
    ui->lblOpDesc->setText(QString::fromStdString(op->GetDescription()));
    ui->textDoc->setMarkdown(QString::fromStdString(op->GetPrincipleDoc()));

    BuildParamControls(op);
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

void PropertyPanel::BuildParamControls(IOpNode* opNode)
{
    ClearDynamicWidgets();

    if (!opNode)
    {
        return;
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

            int minVal = std::holds_alternative<int>(param.MinValue) ? std::get<int>(param.MinValue) : 0;
            int maxVal = std::holds_alternative<int>(param.MaxValue) ? std::get<int>(param.MaxValue) : 100;
            int step = std::holds_alternative<int>(param.Step) ? std::get<int>(param.Step) : 1;
            int curVal = param.GetInt();

            slider->setRange(minVal, maxVal);
            slider->setSingleStep(step);
            slider->setValue(curVal);

            spinBox->setRange(minVal, maxVal);
            spinBox->setSingleStep(step);
            spinBox->setValue(curVal);

            connect(slider, &QSlider::valueChanged, spinBox, &QSpinBox::setValue);
            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, paramName](int val) {
                if (_pipelineMgr && _currentStepIndex >= 0)
                {
                    PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                    if (s && s->GetOpNode())
                    {
                        s->GetOpNode()->SetParameter(paramName, val);
                        _pipelineMgr->MarkDirtyFrom(_currentStepIndex);
                        _pipelineMgr->Execute();
                        emit ParameterChanged(_currentStepIndex);
                    }
                }
            });

            ctrlLayout->addWidget(slider, 1);
            ctrlLayout->addWidget(spinBox);
            rowLayout->addLayout(ctrlLayout);
        }
        else if (param.Type == ParamType::DoubleSlider)
        {
            auto* ctrlLayout = new QHBoxLayout();
            auto* slider = new QSlider(Qt::Horizontal, rowWidget);
            auto* spinBox = new QDoubleSpinBox(rowWidget);

            double minVal = param.MinValue.index() == 0 ? std::get<int>(param.MinValue) : std::get<double>(param.MinValue);
            double maxVal = param.MaxValue.index() == 0 ? std::get<int>(param.MaxValue) : std::get<double>(param.MaxValue);
            double step = param.Step.index() == 0 ? std::get<int>(param.Step) : std::get<double>(param.Step);
            double curVal = param.GetDouble();

            int scale = 100;
            slider->setRange(static_cast<int>(minVal * scale), static_cast<int>(maxVal * scale));
            slider->setValue(static_cast<int>(curVal * scale));

            spinBox->setRange(minVal, maxVal);
            spinBox->setSingleStep(step);
            spinBox->setDecimals(2);
            spinBox->setValue(curVal);

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
                        _pipelineMgr->MarkDirtyFrom(_currentStepIndex);
                        _pipelineMgr->Execute();
                        emit ParameterChanged(_currentStepIndex);
                    }
                }
            });

            ctrlLayout->addWidget(slider, 1);
            ctrlLayout->addWidget(spinBox);
            rowLayout->addLayout(ctrlLayout);
        }
        else if (param.Type == ParamType::ComboBox)
        {
            auto* combo = new QComboBox(rowWidget);
            for (const auto& opt : param.EnumOptions)
            {
                combo->addItem(QString::fromStdString(opt));
            }
            combo->setCurrentIndex(param.GetInt());

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, paramName](int idx) {
                if (_pipelineMgr && _currentStepIndex >= 0)
                {
                    PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                    if (s && s->GetOpNode())
                    {
                        s->GetOpNode()->SetParameter(paramName, idx);
                        _pipelineMgr->MarkDirtyFrom(_currentStepIndex);
                        _pipelineMgr->Execute();
                        emit ParameterChanged(_currentStepIndex);
                    }
                }
            });

            rowLayout->addWidget(combo);
        }
        else if (param.Type == ParamType::CheckBox)
        {
            auto* chk = new QCheckBox(QString::fromStdString(param.DisplayName), rowWidget);
            chk->setChecked(param.GetBool());

            connect(chk, &QCheckBox::toggled, [this, paramName](bool checked) {
                if (_pipelineMgr && _currentStepIndex >= 0)
                {
                    PipelineStep* s = _pipelineMgr->GetStep(_currentStepIndex);
                    if (s && s->GetOpNode())
                    {
                        s->GetOpNode()->SetParameter(paramName, checked);
                        _pipelineMgr->MarkDirtyFrom(_currentStepIndex);
                        _pipelineMgr->Execute();
                        emit ParameterChanged(_currentStepIndex);
                    }
                }
            });

            rowLayout->addWidget(chk);
        }

        ui->dynamicParamsLayout->addWidget(rowWidget);
    }

    ui->dynamicParamsLayout->addStretch();
}
