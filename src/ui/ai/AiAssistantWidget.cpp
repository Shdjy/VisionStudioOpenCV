#include "AiAssistantWidget.h"
#include "ui_AiAssistantWidget.h"
#include "AiSettingsDialog.h"
#include "../../ai/OperatorKnowledgeBase.h"
#include "../../core/OpRegistry.h"
#include "../../utils/IconUtils.h"
#include "../../utils/Logger.h"
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

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

    // 设置科技感图标
    ui->btnOpenSettings->setIcon(IconUtils::CreateSettingsGearIcon(QColor(100, 116, 139)));
    ui->btnApplyTask->setIcon(IconUtils::CreateInspectionReticleIcon(Qt::white));
    ui->btnApplyPipeline->setIcon(IconUtils::CreateAiSparkleIcon(Qt::white));

    InitUiAndConfig();

    // 绑定信号槽
    connect(ui->btnOpenSettings, &QPushButton::clicked, this, &AiAssistantWidget::OnOpenSettingsClicked);
    connect(ui->cbActiveProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AiAssistantWidget::OnProviderChanged);
    connect(ui->cbModel, &QComboBox::currentTextChanged, this, &AiAssistantWidget::OnModelChanged);
    connect(ui->btnClearChat, &QPushButton::clicked, this, &AiAssistantWidget::OnClearChatClicked);
    connect(ui->btnGenerate, &QPushButton::clicked, this, &AiAssistantWidget::OnGeneratePlanClicked);
    connect(ui->btnApplyTask, &QPushButton::clicked, this, &AiAssistantWidget::OnApplyDetectionTaskClicked);
    connect(ui->btnApplyPipeline, &QPushButton::clicked, this, &AiAssistantWidget::OnApplyPipelineClicked);
    connect(ui->btnToggleThinking, &QPushButton::clicked, this, &AiAssistantWidget::OnToggleThinkingClicked);
    connect(ui->btnTplCount, &QPushButton::clicked, this, &AiAssistantWidget::OnQuickTemplateCountClicked);
    connect(ui->btnTplDefect, &QPushButton::clicked, this, &AiAssistantWidget::OnQuickTemplateDefectClicked);
    connect(ui->btnTplMeasure, &QPushButton::clicked, this, &AiAssistantWidget::OnQuickTemplateMeasureClicked);
}

AiAssistantWidget::~AiAssistantWidget()
{
    delete _planner;
    delete _provider;
    delete ui;
}

void AiAssistantWidget::InitUiAndConfig()
{
    RefreshProviderListFromConfig();

    // 显示默认欢迎卡片
    OnClearChatClicked();
}

void AiAssistantWidget::RefreshProviderListFromConfig()
{
    AiConfigManager& cfg = AiConfigManager::Instance();
    auto profiles = cfg.GetProfiles();
    QString activeId = cfg.GetActiveProfileId();

    ui->cbActiveProvider->blockSignals(true);
    ui->cbActiveProvider->clear();

    int activeIdx = 0;
    for (size_t i = 0; i < profiles.size(); ++i)
    {
        ui->cbActiveProvider->addItem(profiles[i].name, profiles[i].id);
        if (profiles[i].id == activeId)
        {
            activeIdx = static_cast<int>(i);
        }
    }

    if (ui->cbActiveProvider->count() > 0)
    {
        ui->cbActiveProvider->setCurrentIndex(activeIdx);
    }
    ui->cbActiveProvider->blockSignals(false);

    // 获取当前激活 Profile
    AiProviderProfile activeProfile = cfg.GetActiveProfile();

    if (!_provider)
    {
        _provider = new OpenAiCompatibleProvider(activeProfile.baseUrl, activeProfile.apiKey, this);
        _planner = new AiDetectionPlanner(_provider, this);
    }
    else
    {
        _provider->SetBaseUrl(activeProfile.baseUrl);
        _provider->SetApiKey(activeProfile.apiKey);
        _provider->SetTimeoutMs(activeProfile.timeoutSec * 1000);
    }

    // 填充模型列表
    ui->cbModel->blockSignals(true);
    ui->cbModel->clear();
    for (const QString& m : activeProfile.cachedModels)
    {
        ui->cbModel->addItem(m);
    }
    ui->cbModel->setEditText(activeProfile.selectedModel);
    ui->cbModel->blockSignals(false);
}

void AiAssistantWidget::OnOpenSettingsClicked()
{
    AiSettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        RefreshProviderListFromConfig();
    }
}

void AiAssistantWidget::OnProviderChanged(int index)
{
    if (index < 0)
    {
        return;
    }

    QString id = ui->cbActiveProvider->itemData(index).toString();
    AiConfigManager::Instance().SetActiveProfileId(id);

    AiProviderProfile profile = AiConfigManager::Instance().GetActiveProfile();
    if (_provider)
    {
        _provider->SetBaseUrl(profile.baseUrl);
        _provider->SetApiKey(profile.apiKey);
        _provider->SetTimeoutMs(profile.timeoutSec * 1000);
    }

    ui->cbModel->blockSignals(true);
    ui->cbModel->clear();
    for (const QString& m : profile.cachedModels)
    {
        ui->cbModel->addItem(m);
    }
    ui->cbModel->setEditText(profile.selectedModel);
    ui->cbModel->blockSignals(false);
}

void AiAssistantWidget::OnModelChanged(const QString& model)
{
    if (!model.trimmed().isEmpty())
    {
        AiConfigManager::Instance().SetSelectedModel(model.trimmed());
    }
}

void AiAssistantWidget::OnClearChatClicked()
{
    _accumulatedChatHtml =
        "<div style='background: linear-gradient(135deg, #f3e5f5 0%, #e8eaf6 100%); "
        "border: 1px solid #d1c4e9; border-radius: 8px; padding: 12px; margin-bottom: 12px;'>"
        "<div style='font-size: 14px; font-weight: bold; color: #4a148c; margin-bottom: 6px;'>"
        "✨ VisionStudio 工业视觉 Agent 规划助手"
        "</div>"
        "<div style='font-size: 12px; color: #424242; line-height: 1.6;'>"
        "欢迎使用全自主视觉方案 Agent！您可以自然语言输入检测需求，也可以结合当前加载的工件图像进行多模态联合分析。<br>"
        "Agent 将在下方输出<b>中途推理思维链 (Thinking Process)</b>、算法拓扑方案与可执行的检测配置。"
        "</div>"
        "</div>";

    ui->chatBrowser->setHtml(_accumulatedChatHtml);
    _lastPlan = ValidatedAiPlan();
    UpdateActionCardStatus();
}

void AiAssistantWidget::SetCurrentSourceImage(const cv::Mat& img)
{
    _currentImage = img;
    if (!img.empty())
    {
        ui->lblImageInfo->setText(QString("(已载入: %1x%2 像素)").arg(img.cols).arg(img.rows));
        ui->lblImageInfo->setStyleSheet("color: #2e7d32; font-weight: bold; font-size: 11px;");
    }
    else
    {
        ui->lblImageInfo->setText("(未载入图像)");
        ui->lblImageInfo->setStyleSheet("color: #757575; font-size: 11px;");
    }
}

void AiAssistantWidget::AppendUserMessage(const QString& text)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString userHtml = QString(
        "<div style='margin-top: 10px; margin-bottom: 12px; display: flex; justify-content: flex-end;'>"
        "<div style='background-color: #e3f2fd; border: 1px solid #bbdefb; border-radius: 8px; padding: 10px 14px; max-width: 90%; float: right; clear: both;'>"
        "<div style='font-size: 11px; color: #1976d2; font-weight: bold; margin-bottom: 4px;'>"
        "👤 需求提出 [%1]:"
        "</div>"
        "<div style='font-size: 13px; color: #0d47a1; white-space: pre-wrap; word-break: break-word;'>"
        "%2"
        "</div>"
        "</div>"
        "<div style='clear: both;'></div>"
        "</div>"
    ).arg(timeStr, text.toHtmlEscaped());

    _accumulatedChatHtml += userHtml;
    ui->chatBrowser->setHtml(_accumulatedChatHtml);
    ui->chatBrowser->verticalScrollBar()->setValue(ui->chatBrowser->verticalScrollBar()->maximum());
}

QString AiAssistantWidget::FormatMarkdownToHtml(const QString& markdown)
{
    // 简易但优美的 Markdown -> HTML 格式化
    QString html = markdown.toHtmlEscaped();

    // 标题
    html.replace(QRegularExpression("^### (.*)$", QRegularExpression::MultilineOption), "<h4 style='color: #283593; margin: 8px 0 4px 0;'>\\1</h4>");
    html.replace(QRegularExpression("^## (.*)$", QRegularExpression::MultilineOption), "<h3 style='color: #1a237e; margin: 10px 0 6px 0; border-bottom: 1px solid #e0e0e0; padding-bottom: 3px;'>\\1</h3>");
    html.replace(QRegularExpression("^# (.*)$", QRegularExpression::MultilineOption), "<h2 style='color: #0d47a1; margin: 12px 0 8px 0;'>\\1</h2>");

    // 粗体
    html.replace(QRegularExpression("\\*\\*(.*?)\\*\\*"), "<b>\\1</b>");

    // 列表项
    html.replace(QRegularExpression("^[\\*-] (.*)$", QRegularExpression::MultilineOption), "<li style='margin-left: 15px;'>\\1</li>");

    // 行内代码
    html.replace(QRegularExpression("`([^`]+)`"), "<code style='background-color: #f1f3f4; color: #c2185b; padding: 1px 4px; border-radius: 3px; font-family: monospace;'>\\1</code>");

    // 换行
    html.replace("\n", "<br>");

    return html;
}

void AiAssistantWidget::AppendAgentMessage(
    const QString& modelName,
    const QString& thinkingProcess,
    const QString& markdownSolution,
    bool isSuccess,
    const QString& errorMessage)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");

    QString thinkingHtml;
    if (!thinkingProcess.isEmpty() && _showThinking)
    {
        thinkingHtml = QString(
            "<div style='background-color: #f8f6fc; border-left: 3px solid #7c4dff; border-radius: 4px; padding: 8px 12px; margin: 8px 0;'>"
            "<div style='font-size: 11px; font-weight: bold; color: #651fff; margin-bottom: 4px;'>"
            "🧠 推理与思考过程 (Thinking Process):"
            "</div>"
            "<div style='font-size: 11.5px; color: #4a148c; font-style: italic; white-space: pre-wrap; line-height: 1.4;'>"
            "%1"
            "</div>"
            "</div>"
        ).arg(thinkingProcess.toHtmlEscaped());
    }

    QString bodyHtml;
    if (isSuccess)
    {
        bodyHtml = FormatMarkdownToHtml(markdownSolution);
    }
    else
    {
        bodyHtml = QString("<div style='color: #d32f2f; font-weight: bold;'>❌ 生成或校验失败: %1</div>")
                       .arg(errorMessage.toHtmlEscaped());
    }

    QString agentHtml = QString(
        "<div style='margin-top: 10px; margin-bottom: 14px; clear: both;'>"
        "<div style='background-color: #ffffff; border: 1px solid #e0e0e0; border-top: 2px solid #7e57c2; border-radius: 8px; padding: 12px; box-shadow: 0 1px 3px rgba(0,0,0,0.05);'>"
        "<div style='font-size: 12px; font-weight: bold; color: #512da8; margin-bottom: 6px; display: flex; justify-content: space-between;'>"
        "<span>✨ Vision Agent · %1</span>"
        "<span style='color: #9e9e9e; font-weight: normal; font-size: 11px;'>%2</span>"
        "</div>"
        "%3"
        "<div style='font-size: 13px; color: #212121; margin-top: 8px; line-height: 1.5;'>"
        "%4"
        "</div>"
        "</div>"
        "</div>"
    ).arg(modelName, timeStr, thinkingHtml, bodyHtml);

    _accumulatedChatHtml += agentHtml;
    ui->chatBrowser->setHtml(_accumulatedChatHtml);
    ui->chatBrowser->verticalScrollBar()->setValue(ui->chatBrowser->verticalScrollBar()->maximum());
}

void AiAssistantWidget::OnGeneratePlanClicked()
{
    QString prompt = ui->txtRequirement->toPlainText().trimmed();
    if (prompt.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先在下方输入框中描述您的检测需求。");
        return;
    }

    QString model = ui->cbModel->currentText().trimmed();
    if (model.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先选择或输入可用的大模型标识。");
        return;
    }

    AppendUserMessage(prompt);
    ui->txtRequirement->clear();

    ui->btnGenerate->setEnabled(false);
    ui->btnGenerate->setText("🧠 正在深度思考\n并规划拓扑...");

    // 展示正在思考中的临时动态卡片
    QString intermediateThinkingHtml = QString(
        "<div id='temp_thinking' style='margin-top: 8px; margin-bottom: 8px; background-color: #f3e5f5; border: 1px dashed #ab47bc; border-radius: 6px; padding: 10px;'>"
        "<div style='font-size: 12px; font-weight: bold; color: #6a1b9a;'>"
        "⏳ Agent 正在调用模型 [%1] 进行视觉推理与拓扑求解..."
        "</div>"
        "<div style='font-size: 11px; color: #8e24aa; margin-top: 4px;'>"
        "• 提取需求关键词与工件特征<br>"
        "• 校验本地算子白名单与参数边界<br>"
        "• 正在接收中途推理思考思维链..."
        "</div>"
        "</div>"
    ).arg(model);

    ui->chatBrowser->setHtml(_accumulatedChatHtml + intermediateThinkingHtml);
    ui->chatBrowser->verticalScrollBar()->setValue(ui->chatBrowser->verticalScrollBar()->maximum());

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
        [this, model](const ValidatedAiPlan& plan) {
            ui->btnGenerate->setEnabled(true);
            ui->btnGenerate->setText("✨ 发送\n生成方案");

            _lastPlan = plan;
            AppendAgentMessage(model, plan.ThinkingProcess, plan.AnalysisMarkdown, plan.IsValid, plan.ErrorMessage);
            UpdateActionCardStatus();

            if (!plan.IsValid)
            {
                QMessageBox::critical(this, "安全校验拦截", "AI 生成方案未能通过本地安全校验:\n" + plan.ErrorMessage);
            }
        }
    );
}

void AiAssistantWidget::UpdateActionCardStatus()
{
    ui->btnApplyTask->setEnabled(_lastPlan.IsValid && _lastPlan.HasDetectionTask);
    ui->btnApplyPipeline->setEnabled(_lastPlan.IsValid && _lastPlan.HasPipeline);
}

void AiAssistantWidget::OnToggleThinkingClicked()
{
    _showThinking = !_showThinking;
    ui->btnToggleThinking->setText(_showThinking ? "🧠 隐藏思考链" : "🧠 显示思考链");

    // 刷新显示
    if (_lastPlan.IsValid)
    {
        // 保持界面当前呈现
    }
}

void AiAssistantWidget::OnApplyDetectionTaskClicked()
{
    if (!_lastPlan.IsValid || !_lastPlan.HasDetectionTask)
    {
        QMessageBox::warning(this, "提示", "当前方案中不包含合法的检测任务配置。");
        return;
    }

    if (_taskManager)
    {
        int idx = _taskManager->AddTask(_lastPlan.Task);
        _taskManager->SetActiveTaskIndex(idx);
        emit DetectionTaskApplied();
        QMessageBox::information(
            this,
            "应用成功",
            "已成功将 AI 生成的任务添加到检测面板:\n" + QString::fromStdString(_lastPlan.Task.Name));
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
    QMessageBox::information(
        this,
        "应用成功",
        QString("已成功将 %1 个算子步骤追加至当前主流水线！").arg(addedCount));
}

void AiAssistantWidget::OnQuickTemplateCountClicked()
{
    ui->txtRequirement->setPlainText(OperatorKnowledgeBase::GetObjectCountingPromptExample());
}

void AiAssistantWidget::OnQuickTemplateDefectClicked()
{
    ui->txtRequirement->setPlainText(OperatorKnowledgeBase::GetDefectDetectionPromptExample());
}

void AiAssistantWidget::OnQuickTemplateMeasureClicked()
{
    ui->txtRequirement->setPlainText(
        "需要对视口中的圆柱形工件进行高精度外径测量与圆度公差检测。\n"
        "要求：检测直径公差在 45.0mm 到 55.0mm 之间，超出范围判定为 NG。");
}
