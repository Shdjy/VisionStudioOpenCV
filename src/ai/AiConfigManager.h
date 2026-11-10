#pragma once

#include <QString>
#include <QSettings>

/**
 * @brief AI 助手配置持久化管理类 (单例)
 */
class AiConfigManager
{
public:
    static AiConfigManager& Instance()
    {
        static AiConfigManager instance;
        return instance;
    }

    QString GetBaseUrl() const;
    void SetBaseUrl(const QString& url);

    QString GetApiKey() const;
    void SetApiKey(const QString& key);

    QString GetSelectedModel() const;
    void SetSelectedModel(const QString& model);

    double GetTemperature() const;
    void SetTemperature(double temp);

    int GetMaxTokens() const;
    void SetMaxTokens(int tokens);

    int GetTimeoutSec() const;
    void SetTimeoutSec(int sec);

    void ResetToDefaults();

private:
    AiConfigManager();
    ~AiConfigManager() = default;
    AiConfigManager(const AiConfigManager&) = delete;
    AiConfigManager& operator=(const AiConfigManager&) = delete;

    QString _baseUrl;
    QString _apiKey;
    QString _selectedModel;
    double _temperature = 0.2;
    int _maxTokens = 2048;
    int _timeoutSec = 60;

    void LoadSettings();
    void SaveSettings();
};
