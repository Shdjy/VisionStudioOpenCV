#include "AiSettingsDialog.h"
#include "ui_AiSettingsDialog.h"
#include "../../utils/IconUtils.h"
#include <QMessageBox>
#include <QDateTime>

AiSettingsDialog::AiSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AiSettingsDialog)
{
    ui->setupUi(this);

    // 设置窗口属性与图标
    setWindowIcon(IconUtils::CreateSettingsGearIcon(QColor(25, 118, 210)));
    resize(820, 580);

    // 初始化下拉框选项
    ui->cbProviderType->addItem("OpenAI 兼容协议 (OpenAI-Compatible)", "OpenAI-Compatible");
    ui->cbProviderType->addItem("DeepSeek 官方 API (DeepSeek)", "DeepSeek");
    ui->cbProviderType->addItem("OpenAI 官方 API (OpenAI)", "OpenAI");
    ui->cbProviderType->addItem("Ollama 本地私有化大模型 (Ollama)", "Ollama");
    ui->cbProviderType->addItem("自定义聚合与代理网关 (Custom)", "Custom");

    ui->cbPresetTemplate->addItem("-- 选择快捷填入模板 --");
    ui->cbPresetTemplate->addItem("TokensKingdom 聚合服务");
    ui->cbPresetTemplate->addItem("DeepSeek 官方开放平台");
    ui->cbPresetTemplate->addItem("OpenAI 官方服务");
    ui->cbPresetTemplate->addItem("Ollama 本地大模型 (11434)");

    // 连接信号槽
    connect(ui->listProfiles, &QListWidget::currentRowChanged, this, &AiSettingsDialog::OnProfileSelectionChanged);
    connect(ui->btnAddProfile, &QPushButton::clicked, this, &AiSettingsDialog::OnAddProfileClicked);
    connect(ui->btnDeleteProfile, &QPushButton::clicked, this, &AiSettingsDialog::OnDeleteProfileClicked);
    connect(ui->btnSetActive, &QPushButton::clicked, this, &AiSettingsDialog::OnSetActiveProfileClicked);
    connect(ui->btnTestConnection, &QPushButton::clicked, this, &AiSettingsDialog::OnTestConnectivityClicked);
    connect(ui->btnFetchModels, &QPushButton::clicked, this, &AiSettingsDialog::OnFetchModelsClicked);
    connect(ui->btnToggleKey, &QPushButton::clicked, this, &AiSettingsDialog::OnToggleApiKeyVisibility);
    connect(ui->cbPresetTemplate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AiSettingsDialog::OnPresetTemplateSelected);
    connect(ui->btnSave, &QPushButton::clicked, this, &AiSettingsDialog::OnSaveAndApplyClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->btnResetDefaults, &QPushButton::clicked, this, &AiSettingsDialog::OnResetToDefaultsClicked);

    LoadProfilesToUi();
}

AiSettingsDialog::~AiSettingsDialog()
{
    delete ui;
}

void AiSettingsDialog::LoadProfilesToUi()
{
    AiConfigManager& mgr = AiConfigManager::Instance();
    _workingProfiles = mgr.GetProfiles();
    _activeProfileId = mgr.GetActiveProfileId();

    RefreshListWidgets();

    // 默认定位到激活项
    int targetRow = 0;
    for (size_t i = 0; i < _workingProfiles.size(); ++i)
    {
        if (_workingProfiles[i].id == _activeProfileId)
        {
            targetRow = static_cast<int>(i);
            break;
        }
    }

    if (ui->listProfiles->count() > 0)
    {
        ui->listProfiles->setCurrentRow(targetRow);
    }
}

void AiSettingsDialog::RefreshListWidgets()
{
    ui->listProfiles->blockSignals(true);
    ui->listProfiles->clear();

    for (size_t i = 0; i < _workingProfiles.size(); ++i)
    {
        const auto& p = _workingProfiles[i];
        bool isActive = (p.id == _activeProfileId);

        QString statusMarker = isActive ? "[当前激活] " : "";
        QString latencyStr;
        if (p.lastLatencyMs >= 0)
        {
            latencyStr = QString(" (%1ms)").arg(p.lastLatencyMs);
        }

        QString itemText = QString("%1%2%3\n  └ %4")
                               .arg(statusMarker)
                               .arg(p.name)
                               .arg(latencyStr)
                               .arg(AiProviderProfile::ProviderTypeToString(p.type));

        QListWidgetItem* item = new QListWidgetItem(itemText);
        if (isActive)
        {
            item->setForeground(QBrush(QColor(46, 125, 50)));
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
        ui->listProfiles->addItem(item);
    }
    ui->listProfiles->blockSignals(false);
}

void AiSettingsDialog::OnProfileSelectionChanged(int currentRow)
{
    if (_currentEditingIndex >= 0 && _currentEditingIndex < static_cast<int>(_workingProfiles.size()))
    {
        SaveCurrentFormToProfile();
    }

    if (currentRow < 0 || currentRow >= static_cast<int>(_workingProfiles.size()))
    {
        return;
    }

    _currentEditingIndex = currentRow;
    DisplayProfile(currentRow);
}

void AiSettingsDialog::DisplayProfile(int index)
{
    if (index < 0 || index >= static_cast<int>(_workingProfiles.size()))
    {
        return;
    }

    const auto& p = _workingProfiles[index];

    ui->txtName->setText(p.name);

    int typeIdx = ui->cbProviderType->findData(AiProviderProfile::ProviderTypeToString(p.type));
    if (typeIdx >= 0)
    {
        ui->cbProviderType->setCurrentIndex(typeIdx);
    }

    ui->txtBaseUrl->setText(p.baseUrl);
    ui->txtApiKey->setText(p.apiKey);

    ui->cbModel->blockSignals(true);
    ui->cbModel->clear();
    for (const QString& m : p.cachedModels)
    {
        ui->cbModel->addItem(m);
    }
    ui->cbModel->setCurrentText(p.selectedModel);
    ui->cbModel->blockSignals(false);

    ui->spinTemperature->setValue(p.temperature);
    ui->spinMaxTokens->setValue(p.maxTokens);
    ui->spinTimeout->setValue(p.timeoutSec);

    if (p.lastLatencyMs >= 0)
    {
        ui->lblTestResult->setStyleSheet("color: #2e7d32; font-weight: bold; font-size: 11px;");
        ui->lblTestResult->setText(QString("✅ 上次连通正常 (延迟: %1 ms)").arg(p.lastLatencyMs));
    }
    else
    {
        ui->lblTestResult->setStyleSheet("color: #757575; font-size: 11px;");
        ui->lblTestResult->setText("尚未进行连接测试");
    }

    ui->lblModelStatus->setText(QString("本地已缓存 %1 个可用模型").arg(p.cachedModels.size()));
}

void AiSettingsDialog::SaveCurrentFormToProfile()
{
    if (_currentEditingIndex < 0 || _currentEditingIndex >= static_cast<int>(_workingProfiles.size()))
    {
        return;
    }

    auto& p = _workingProfiles[_currentEditingIndex];
    p.name = ui->txtName->text().trimmed();
    p.type = AiProviderProfile::StringToProviderType(ui->cbProviderType->currentData().toString());
    p.baseUrl = ui->txtBaseUrl->text().trimmed();
    p.apiKey = ui->txtApiKey->text().trimmed();
    p.selectedModel = ui->cbModel->currentText().trimmed();
    p.temperature = ui->spinTemperature->value();
    p.maxTokens = ui->spinMaxTokens->value();
    p.timeoutSec = ui->spinTimeout->value();
}

void AiSettingsDialog::OnAddProfileClicked()
{
    SaveCurrentFormToProfile();

    AiProviderProfile np;
    np.id = "provider_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    np.name = "新服务商配置 (" + QString::number(_workingProfiles.size() + 1) + ")";
    np.type = AiProviderType::OpenAiCompatible;
    np.baseUrl = "https://api.openai.com/v1";
    np.apiKey = "";
    np.selectedModel = "gpt-4o";
    np.cachedModels = { "gpt-4o", "gpt-4o-mini" };
    np.temperature = 0.2;
    np.maxTokens = 2048;
    np.timeoutSec = 60;
    np.isEnabled = true;
    np.lastLatencyMs = -1;

    _workingProfiles.push_back(np);

    RefreshListWidgets();
    ui->listProfiles->setCurrentRow(static_cast<int>(_workingProfiles.size() - 1));
}

void AiSettingsDialog::OnDeleteProfileClicked()
{
    if (_workingProfiles.size() <= 1)
    {
        QMessageBox::warning(this, "提示", "至少需要保留一个服务商配置，无法删除最后一个。");
        return;
    }

    if (_currentEditingIndex < 0 || _currentEditingIndex >= static_cast<int>(_workingProfiles.size()))
    {
        return;
    }

    QString toDeleteId = _workingProfiles[_currentEditingIndex].id;
    auto reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除服务商配置 [%1] 吗？").arg(_workingProfiles[_currentEditingIndex].name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
    {
        return;
    }

    _workingProfiles.erase(_workingProfiles.begin() + _currentEditingIndex);
    if (_activeProfileId == toDeleteId)
    {
        _activeProfileId = _workingProfiles.front().id;
    }

    _currentEditingIndex = -1;
    RefreshListWidgets();
    ui->listProfiles->setCurrentRow(0);
}

void AiSettingsDialog::OnSetActiveProfileClicked()
{
    if (_currentEditingIndex < 0 || _currentEditingIndex >= static_cast<int>(_workingProfiles.size()))
    {
        return;
    }

    SaveCurrentFormToProfile();
    _activeProfileId = _workingProfiles[_currentEditingIndex].id;
    RefreshListWidgets();
    ui->listProfiles->setCurrentRow(_currentEditingIndex);
}

void AiSettingsDialog::OnToggleApiKeyVisibility()
{
    _isApiKeyVisible = !_isApiKeyVisible;
    if (_isApiKeyVisible)
    {
        ui->txtApiKey->setEchoMode(QLineEdit::Normal);
        ui->btnToggleKey->setText("隐藏");
    }
    else
    {
        ui->txtApiKey->setEchoMode(QLineEdit::Password);
        ui->btnToggleKey->setText("显示");
    }
}

void AiSettingsDialog::OnPresetTemplateSelected(int index)
{
    if (index <= 0)
    {
        return;
    }

    if (index == 1) // TokensKingdom
    {
        ui->txtName->setText("TokensKingdom 聚合服务");
        ui->cbProviderType->setCurrentIndex(ui->cbProviderType->findData("OpenAI-Compatible"));
        ui->txtBaseUrl->setText("https://api.tokenskingdom.com/v1");
        ui->cbModel->setEditText("gpt-6.1-sol");
    }
    else if (index == 2) // DeepSeek
    {
        ui->txtName->setText("DeepSeek 官方 API");
        ui->cbProviderType->setCurrentIndex(ui->cbProviderType->findData("DeepSeek"));
        ui->txtBaseUrl->setText("https://api.deepseek.com/v1");
        ui->cbModel->setEditText("deepseek-chat");
    }
    else if (index == 3) // OpenAI
    {
        ui->txtName->setText("OpenAI 官方 API");
        ui->cbProviderType->setCurrentIndex(ui->cbProviderType->findData("OpenAI"));
        ui->txtBaseUrl->setText("https://api.openai.com/v1");
        ui->cbModel->setEditText("gpt-4o");
    }
    else if (index == 4) // Ollama
    {
        ui->txtName->setText("Ollama 本地私有化模型");
        ui->cbProviderType->setCurrentIndex(ui->cbProviderType->findData("Ollama"));
        ui->txtBaseUrl->setText("http://localhost:11434/v1");
        ui->txtApiKey->setText("ollama");
        ui->cbModel->setEditText("deepseek-r1:14b");
    }

    ui->cbPresetTemplate->setCurrentIndex(0);
}

void AiSettingsDialog::OnTestConnectivityClicked()
{
    SaveCurrentFormToProfile();

    QString baseUrl = ui->txtBaseUrl->text().trimmed();
    QString apiKey = ui->txtApiKey->text().trimmed();

    if (baseUrl.isEmpty())
    {
        QMessageBox::warning(this, "配置不完整", "请输入有效的 API 基础地址 (Base URL)。");
        return;
    }

    ui->btnTestConnection->setEnabled(false);
    ui->lblTestResult->setStyleSheet("color: #0288d1; font-size: 11px;");
    ui->lblTestResult->setText("⏳ 正在发起网络连通性测试与延迟测量...");

    OpenAiCompatibleProvider* testProvider = new OpenAiCompatibleProvider(baseUrl, apiKey, this);
    testProvider->SetTimeoutMs(ui->spinTimeout->value() * 1000);

    testProvider->TestConnectivityAsync([this, testProvider](bool success, qint64 latencyMs, const QStringList& models, const QString& error) {
        ui->btnTestConnection->setEnabled(true);
        testProvider->deleteLater();

        if (!success)
        {
            ui->lblTestResult->setStyleSheet("color: #d32f2f; font-weight: bold; font-size: 11px;");
            ui->lblTestResult->setText(QString("❌ 连接失败 (%1 ms): %2").arg(latencyMs).arg(error));
            if (_currentEditingIndex >= 0 && _currentEditingIndex < static_cast<int>(_workingProfiles.size()))
            {
                _workingProfiles[_currentEditingIndex].lastLatencyMs = -1;
            }
            return;
        }

        ui->lblTestResult->setStyleSheet("color: #2e7d32; font-weight: bold; font-size: 11px;");
        ui->lblTestResult->setText(QString("✅ 连通测试通过！往返网络延迟: %1 ms").arg(latencyMs));

        if (_currentEditingIndex >= 0 && _currentEditingIndex < static_cast<int>(_workingProfiles.size()))
        {
            auto& p = _workingProfiles[_currentEditingIndex];
            p.lastLatencyMs = latencyMs;

            if (!models.isEmpty())
            {
                p.cachedModels = models;
                ui->cbModel->blockSignals(true);
                ui->cbModel->clear();
                ui->cbModel->addItems(models);
                if (!models.contains(p.selectedModel))
                {
                    ui->cbModel->setCurrentIndex(0);
                    p.selectedModel = models.first();
                }
                else
                {
                    ui->cbModel->setCurrentText(p.selectedModel);
                }
                ui->cbModel->blockSignals(false);
                ui->lblModelStatus->setText(QString("在线获取到 %1 个模型并已缓存").arg(models.size()));
            }
        }

        // 刷新列表上的延迟显示
        RefreshListWidgets();
        ui->listProfiles->setCurrentRow(_currentEditingIndex);
    });
}

void AiSettingsDialog::OnFetchModelsClicked()
{
    SaveCurrentFormToProfile();

    QString baseUrl = ui->txtBaseUrl->text().trimmed();
    QString apiKey = ui->txtApiKey->text().trimmed();

    if (baseUrl.isEmpty())
    {
        QMessageBox::warning(this, "配置不完整", "请输入有效的 API 基础地址。");
        return;
    }

    ui->btnFetchModels->setEnabled(false);
    ui->lblModelStatus->setStyleSheet("color: #0288d1; font-size: 11px;");
    ui->lblModelStatus->setText("⏳ 正在从服务端拉取模型列表中...");

    OpenAiCompatibleProvider* fetchProvider = new OpenAiCompatibleProvider(baseUrl, apiKey, this);
    fetchProvider->SetTimeoutMs(ui->spinTimeout->value() * 1000);

    fetchProvider->FetchModelsAsync([this, fetchProvider](bool success, const QStringList& models, const QString& error) {
        ui->btnFetchModels->setEnabled(true);
        fetchProvider->deleteLater();

        if (!success)
        {
            ui->lblModelStatus->setStyleSheet("color: #d32f2f; font-size: 11px;");
            ui->lblModelStatus->setText("拉取模型失败: " + error);
            QMessageBox::warning(this, "拉取失败", "无法获取模型列表:\n" + error);
            return;
        }

        ui->cbModel->blockSignals(true);
        ui->cbModel->clear();
        ui->cbModel->addItems(models);

        if (_currentEditingIndex >= 0 && _currentEditingIndex < static_cast<int>(_workingProfiles.size()))
        {
            auto& p = _workingProfiles[_currentEditingIndex];
            p.cachedModels = models;
            if (!models.contains(p.selectedModel) && !models.isEmpty())
            {
                p.selectedModel = models.first();
                ui->cbModel->setCurrentIndex(0);
            }
            else
            {
                ui->cbModel->setCurrentText(p.selectedModel);
            }
        }
        ui->cbModel->blockSignals(false);

        ui->lblModelStatus->setStyleSheet("color: #2e7d32; font-size: 11px;");
        ui->lblModelStatus->setText(QString("✅ 成功拉取到 %1 个在线可用模型").arg(models.size()));
    });
}

void AiSettingsDialog::OnResetToDefaultsClicked()
{
    auto res = QMessageBox::question(
        this,
        "确认恢复预设",
        "确定要将所有服务商配置重置为系统出厂预设吗？当前的自定义配置将被覆盖。",
        QMessageBox::Yes | QMessageBox::No);

    if (res != QMessageBox::Yes)
    {
        return;
    }

    AiConfigManager::Instance().ResetToDefaults();
    LoadProfilesToUi();
}

void AiSettingsDialog::OnSaveAndApplyClicked()
{
    SaveCurrentFormToProfile();

    AiConfigManager& mgr = AiConfigManager::Instance();
    mgr.SetProfiles(_workingProfiles);
    mgr.SetActiveProfileId(_activeProfileId);

    emit SettingsChanged();
    accept();
}
