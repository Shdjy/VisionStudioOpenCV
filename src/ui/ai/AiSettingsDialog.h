#pragma once

#include <QDialog>
#include <vector>
#include "../../ai/AiConfigManager.h"
#include "../../ai/OpenAiCompatibleProvider.h"

namespace Ui {
class AiSettingsDialog;
}

/**
 * @brief AI 服务商与大模型配置管理中心 (CC Switch 风格独立设置窗口)
 * 
 * 职责：
 * 1. 集中管理多个 AI 服务商 Profile (OpenAI兼容聚合、DeepSeek、OpenAI、本地Ollama等)
 * 2. 独立设置 Base URL、API Key 与安全密文切换
 * 3. 实时连通性测试与往返延迟 (Latency) 测量
 * 4. 在线模型列表自动拉取与模型选择
 * 5. 一键切换当前激活的生产服务商
 */
class AiSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AiSettingsDialog(QWidget* parent = nullptr);
    ~AiSettingsDialog() override;

signals:
    /**
     * @brief 当服务商配置或当前激活项发生改变并保存时触发
     */
    void SettingsChanged();

private slots:
    void OnProfileSelectionChanged(int currentRow);
    void OnAddProfileClicked();
    void OnDeleteProfileClicked();
    void OnSetActiveProfileClicked();
    void OnTestConnectivityClicked();
    void OnFetchModelsClicked();
    void OnToggleApiKeyVisibility();
    void OnPresetTemplateSelected(int index);
    void OnSaveAndApplyClicked();
    void OnResetToDefaultsClicked();

private:
    Ui::AiSettingsDialog* ui;
    std::vector<AiProviderProfile> _workingProfiles;
    QString _activeProfileId;
    int _currentEditingIndex = -1;
    bool _isApiKeyVisible = false;

    void LoadProfilesToUi();
    void SaveCurrentFormToProfile();
    void DisplayProfile(int index);
    void UpdateListRowAppearance(int row);
    void RefreshListWidgets();
};
