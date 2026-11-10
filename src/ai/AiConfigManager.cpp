#include "AiConfigManager.h"
#include <QCoreApplication>

namespace
{
constexpr const char* DefaultBaseUrl = "https://api.tokenskingdom.com/v1";
constexpr const char* DefaultApiKey = "sk-ec53d4c931f986190063a14a5ab287bfa443971621ce97f52d4622c951811ffb";
constexpr const char* DefaultModel = "gpt-6.1-sol";
}

AiConfigManager::AiConfigManager()
{
    LoadSettings();
}

void AiConfigManager::LoadSettings()
{
    QSettings settings;
    _baseUrl = settings.value("AI/BaseUrl", DefaultBaseUrl).toString();
    _apiKey = settings.value("AI/ApiKey", DefaultApiKey).toString();
    _selectedModel = settings.value("AI/SelectedModel", DefaultModel).toString();
    _temperature = settings.value("AI/Temperature", 0.2).toDouble();
    _maxTokens = settings.value("AI/MaxTokens", 2048).toInt();
    _timeoutSec = settings.value("AI/TimeoutSec", 60).toInt();
}

void AiConfigManager::SaveSettings()
{
    QSettings settings;
    settings.setValue("AI/BaseUrl", _baseUrl);
    settings.setValue("AI/ApiKey", _apiKey);
    settings.setValue("AI/SelectedModel", _selectedModel);
    settings.setValue("AI/Temperature", _temperature);
    settings.setValue("AI/MaxTokens", _maxTokens);
    settings.setValue("AI/TimeoutSec", _timeoutSec);
}

QString AiConfigManager::GetBaseUrl() const
{
    return _baseUrl;
}

void AiConfigManager::SetBaseUrl(const QString& url)
{
    _baseUrl = url.trimmed();
    SaveSettings();
}

QString AiConfigManager::GetApiKey() const
{
    return _apiKey;
}

void AiConfigManager::SetApiKey(const QString& key)
{
    _apiKey = key.trimmed();
    SaveSettings();
}

QString AiConfigManager::GetSelectedModel() const
{
    return _selectedModel;
}

void AiConfigManager::SetSelectedModel(const QString& model)
{
    _selectedModel = model.trimmed();
    SaveSettings();
}

double AiConfigManager::GetTemperature() const
{
    return _temperature;
}

void AiConfigManager::SetTemperature(double temp)
{
    _temperature = temp;
    SaveSettings();
}

int AiConfigManager::GetMaxTokens() const
{
    return _maxTokens;
}

void AiConfigManager::SetMaxTokens(int tokens)
{
    _maxTokens = tokens;
    SaveSettings();
}

int AiConfigManager::GetTimeoutSec() const
{
    return _timeoutSec;
}

void AiConfigManager::SetTimeoutSec(int sec)
{
    _timeoutSec = sec;
    SaveSettings();
}

void AiConfigManager::ResetToDefaults()
{
    _baseUrl = DefaultBaseUrl;
    _apiKey = DefaultApiKey;
    _selectedModel = DefaultModel;
    _temperature = 0.2;
    _maxTokens = 2048;
    _timeoutSec = 60;
    SaveSettings();
}
