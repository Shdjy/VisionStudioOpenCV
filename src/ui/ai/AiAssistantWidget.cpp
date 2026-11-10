#include "AiAssistantWidget.h"
#include "ui_AiAssistantWidget.h"
#include "../../ai/OperatorKnowledgeBase.h"
#include "../../core/OpRegistry.h"
#include "../../utils/Logger.h"
#include <QMessageBox>

AiAssistantWidget::AiAssistantWidget(
    PipelineManager* pipelineManager,
    DetectionTaskManager* taskManager,
    QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AiAssistantWidget)
    , _pipelineManager(pipelineManager)
    , _taskManager(taskManager)
{
    ui->setupUi(this);

    InitUiAndConfig();

    connect(ui->btnFetchModels, &QPushButton::clicked, this, &AiAssistantWidget::OnFetchModelsClicked);
    connect(ui->btnGenerate, &QPushButton::clicked, this, &AiAssistantWidget::OnGeneratePlanClicked);
    connect(ui->btnApplyTask, &QPushButton::clicked, this, &AiAssistantWidget::OnApplyDetectionTaskClicked);
    connect(ui->btnApplyPipeline, &QPushButton::clicked, this, &AiAssistantWidget::OnApplyPipelineClicked);
    connect(ui->btnTplCount, &QPushButton::clicked, this, &AiAssistantWidget::OnQuickTemplateCountClicked);
    connect(ui->btnTplDefect, &QPushButton::clicked, this, &AiAssistantWidget::OnQuickTemplateDefectClicked);
}

AiAssistantWidget::~AiAssistantWidget()
{
    SaveConfigFromUi();
    delete _planner;
    delete _provider;
    delete ui;
}

void AiAssistantWidget::InitUiAndConfig()
{
    AiConfigManager& cfg = AiConfigManager::Instance();
    ui->txtBaseUrl->setText(cfg.GetBaseUrl());
    ui->txtApiKey->setText(cfg.GetApiKey());

    _provider = new OpenAiCompatibleProvider(cfg.GetBaseUrl(), cfg.GetApiKey(), this);
    _planner = new AiDetectionPlanner(_provider, this);

    QString currentModel = cfg.GetSelectedModel();
    if (!currentModel.isEmpty())
    {
        ui->cbModel->addItem(currentModel);
        ui->cbModel->setCurrentText(currentModel);
    }
}

void AiAssistantWidget::SaveConfigFromUi()
{
    AiConfigManager& cfg = AiConfigManager::Instance();
    cfg.SetBaseUrl(ui->txtBaseUrl->text().trimmed());
    cfg.SetApiKey(ui->txtApiKey->text().trimmed());
    cfg.SetSelectedModel(ui->cbModel->currentText().trimmed());

    if (_provider)
    {
        _provider->SetBaseUrl(cfg.GetBaseUrl());
        _provider->SetApiKey(cfg.GetApiKey());
    }
}

void AiAssistantWidget::SetCurrentSourceImage(const cv::Mat& img)
{
    _currentImage = img;
}

void AiAssistantWidget::OnFetchModelsClicked()
{
    SaveConfigFromUi();
    ui->btnFetchModels->setEnabled(false);
    ui->lblModelStatus->setText("正在连接服务端并拉取可用模型列表...");

    _provider->FetchModelsAsync([this](bool success, const QStringList& models, const QString& error) {
        ui->btnFetchModels->setEnabled(true);
        if (!success)
        {
            ui->lblModelStatus->setStyleSheet("color: #f44336; font-size: 11px;");
            ui->lblModelStatus->setText("获取失败: " + error);
            QMessageBox::warning(this, "模型获取失败", "无法连接到 AI 服务端或密钥错误:\n" + error);
            return;
        }

        ui->cbModel->clear();
        ui->cbModel->addItems(models);

        // 优先选择支持良好的可用模型
        int bestIdx = -1;
        for (int i = 0; i < models.size(); ++i)
        {
            if (models[i].contains("gpt-6.1-sol") || models[i].contains("gpt-6-astra"))
            {
                bestIdx = i;
                break;
            }
        }
        if (bestIdx >= 0)
        {
            ui->cbModel->setCurrentIndex(bestIdx);
        }

        ui->lblModelStatus->setStyleSheet("color: #4caf50; font-size: 11px;");
        ui->lblModelStatus->setText(QString("成功获取到 %1 个在线模型").arg(models.size()));
        SaveConfigFromUi();
    });
}

void AiAssistantWidget::OnQuickTemplateCountClicked()
{
    ui->txtRequirement->setText(OperatorKnowledgeBase::GetObjectCountingPromptExample());
}

void AiAssistantWidget::OnQuickTemplateDefectClicked()
{
    ui->txtRequirement->setText(OperatorKnowledgeBase::GetDefectDetectionPromptExample());
}

void AiAssistantWidget::OnGeneratePlanClicked()
{
    QString prompt = ui->txtRequirement->toPlainText().trimmed();
    if (prompt.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先输入检测需求自然语言描述。");
        return;
    }

    SaveConfigFromUi();
    QString model = ui->cbModel->currentText().trimmed();
    if (model.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先点击“自动获取模型”或选择有效的大模型。");
        return;
    }

    ui->btnGenerate->setEnabled(false);
    ui->btnGenerate->setText("正在请求大模型生成算法方案...");
    ui->txtAnalysis->setPlainText("AI 思考与规划中，请稍候...");

    cv::Mat passImage;
    if (ui->chkIncludeImage->isChecked() && !_currentImage.empty())
    {
        passImage = _currentImage;
    }

    _planner->PlanDetectionAsync(
        prompt,
        passImage,
        model,
        AiConfigManager::Instance().GetTemperature(),
        [this](const ValidatedAiPlan& plan) {
            ui->btnGenerate->setEnabled(true);
            ui->btnGenerate->setText("✨ AI 智能规划并生成算法方案");

            _lastPlan = plan;

            if (!plan.IsValid)
            {
                ui->txtAnalysis->setHtml(QString("<font color='red'><b>规划失败或本地校验未通过:</b><br>%1</font>").arg(plan.ErrorMessage));
                ui->txtJsonConfig->setPlainText(plan.RawJsonString);
                QMessageBox::critical(this, "校验错误", "AI 生成的方案未能通过本地安全校验:\n" + plan.ErrorMessage);
                return;
            }

            // 展示 Markdown 格式分析内容
            ui->txtAnalysis->setMarkdown(plan.AnalysisMarkdown);
            ui->txtJsonConfig->setPlainText(plan.RawJsonString);

            // 切换到方案解析标签页
            ui->tabOutput->setCurrentIndex(0);

            QMessageBox::information(this, "规划成功", "AI 方案已生成并通过本地安全白名单校验！\n可点击底部按钮应用到工程中。");
        }
    );
}

void AiAssistantWidget::OnApplyDetectionTaskClicked()
{
    if (!_lastPlan.IsValid || !_lastPlan.HasDetectionTask)
    {
        QMessageBox::warning(this, "提示", "当前方案中不包含合法的检测任务配置，请先生成方案。");
        return;
    }

    if (_taskManager)
    {
        int idx = _taskManager->AddTask(_lastPlan.Task);
        _taskManager->SetActiveTaskIndex(idx);
        emit DetectionTaskApplied();
        QMessageBox::information(this, "成功", "已成功将 AI 生成的任务添加到检测面板:\n" + QString::fromStdString(_lastPlan.Task.Name));
    }
}

void AiAssistantWidget::OnApplyPipelineClicked()
{
    if (!_lastPlan.IsValid || !_lastPlan.HasPipeline)
    {
        QMessageBox::warning(this, "提示", "当前方案中不包含流水线步骤配置。");
        return;
    }

    if (!_pipelineManager)
    {
        return;
    }

    int addedCount = 0;
    for (int i = 0; i < _lastPlan.PipelineSteps.size(); ++i)
    {
        QJsonObject sObj = _lastPlan.PipelineSteps[i].toObject();
        std::string opName = sObj["name"].toString().toStdString();
        auto op = OpRegistry::Instance().Create(opName);
        if (op)
        {
            QJsonObject params = sObj["parameters"].toObject();
            for (auto& p : op->GetParameters())
            {
                QString pName = QString::fromStdString(p.Name);
                if (params.contains(pName))
                {
                    if (p.Type == ParamType::IntSlider || p.Type == ParamType::ComboBox)
                    {
                        op->SetParameter(p.Name, params[pName].toInt());
                    }
                    else if (p.Type == ParamType::DoubleSlider)
                    {
                        op->SetParameter(p.Name, params[pName].toDouble());
                    }
                    else if (p.Type == ParamType::CheckBox)
                    {
                        op->SetParameter(p.Name, params[pName].toBool());
                    }
                }
            }
            _pipelineManager->AddStep(std::move(op));
            addedCount++;
        }
    }

    emit PipelineStepsApplied();
    QMessageBox::information(this, "成功", QString("已成功将 %1 个算子步骤追加至当前流水线！").arg(addedCount));
}
