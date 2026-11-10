#include "AiConfigManager.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace
{
constexpr const char* DefaultBaseUrl = "https://api.tokenskingdom.com/v1";
constexpr const char* DefaultApiKey = "";
constexpr const char* DefaultModel = "gpt-6.1-sol";
constexpr const char* DefaultActiveProfileId = "tokenskingdom";
}

QString AiProviderProfile::ProviderTypeToString(AiProviderType type)
{
    switch (type)
    {
        case AiProviderType::OpenAiCompatible:
            return "OpenAI-Compatible";
        case AiProviderType::DeepSeek:
            return "DeepSeek";
        case AiProviderType::OpenAi:
            return "OpenAI";
        case AiProviderType::Ollama:
            return "Ollama";
        case AiProviderType::Custom:
            return "Custom";
        default:
            return "OpenAI-Compatible";
    }
}

AiProviderType AiProviderProfile::StringToProviderType(const QString& str)
{
    QString s = str.trimmed().toLower();
    if (s == "deepseek")
    {
        return AiProviderType::DeepSeek;
    }
    if (s == "openai")
    {
        return AiProviderType::OpenAi;
    }
    if (s == "ollama")
    {
        return AiProviderType::Ollama;
    }
    if (s == "custom")
    {
        return AiProviderType::Custom;
    }
    return AiProviderType::OpenAiCompatible;
}

QJsonObject AiProviderProfile::ToJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["type"] = ProviderTypeToString(type);
    obj["baseUrl"] = baseUrl;
    obj["apiKey"] = apiKey;
    obj["selectedModel"] = selectedModel;

    QJsonArray modelsArr;
    for (const QString& m : cachedModels)
    {
        modelsArr.append(m);
    }
    obj["cachedModels"] = modelsArr;

    obj["temperature"] = temperature;
    obj["maxTokens"] = maxTokens;
    obj["timeoutSec"] = timeoutSec;
    obj["isEnabled"] = isEnabled;
    obj["lastLatencyMs"] = static_cast<qint64>(lastLatencyMs);

    return obj;
}

AiProviderProfile AiProviderProfile::FromJson(const QJsonObject& obj)
{
    AiProviderProfile p;
    p.id = obj["id"].toString();
    p.name = obj["name"].toString();
    p.type = StringToProviderType(obj["type"].toString());
    p.baseUrl = obj["baseUrl"].toString();
    p.apiKey = obj["apiKey"].toString();
    p.selectedModel = obj["selectedModel"].toString();

    QJsonArray modelsArr = obj["cachedModels"].toArray();
    for (const auto& val : modelsArr)
    {
        p.cachedModels.append(val.toString());
    }

    p.temperature = obj["temperature"].toDouble(0.2);
    p.maxTokens = obj["maxTokens"].toInt(2048);
    p.timeoutSec = obj["timeoutSec"].toInt(60);
    p.isEnabled = obj["isEnabled"].toBool(true);
    p.lastLatencyMs = obj["lastLatencyMs"].toVariant().toLongLong();

    return p;
}

AiConfigManager::AiConfigManager()
{
    LoadSettings();
}

void AiConfigManager::InitDefaultProfiles()
{
    _profiles.clear();

    // 1. TokensKingdom 聚合服务 (用户测试提供的生产服务商)
    AiProviderProfile tk;
    tk.id = "tokenskingdom";
    tk.name = "TokensKingdom 聚合服务 (默认推荐)";
    tk.type = AiProviderType::OpenAiCompatible;
    tk.baseUrl = DefaultBaseUrl;
    tk.apiKey = DefaultApiKey;
    tk.selectedModel = DefaultModel;
    tk.cachedModels = { "gpt-6.1-sol", "gpt-6-astra", "gpt-4o", "deepseek-chat", "deepseek-reasoner" };
    tk.temperature = 0.2;
    tk.maxTokens = 4096;
    tk.timeoutSec = 60;
    tk.isEnabled = true;
    _profiles.push_back(tk);

    // 2. DeepSeek 官方 API
    AiProviderProfile ds;
    ds.id = "deepseek";
    ds.name = "DeepSeek 官方 API (推理强化)";
    ds.type = AiProviderType::DeepSeek;
    ds.baseUrl = "https://api.deepseek.com/v1";
    ds.apiKey = "";
    ds.selectedModel = "deepseek-chat";
    ds.cachedModels = { "deepseek-chat", "deepseek-reasoner" };
    ds.temperature = 0.3;
    ds.maxTokens = 4096;
    ds.timeoutSec = 90;
    ds.isEnabled = true;
    _profiles.push_back(ds);

    // 3. OpenAI 官方 API
    AiProviderProfile oai;
    oai.id = "openai";
    oai.name = "OpenAI 官方 API";
    oai.type = AiProviderType::OpenAi;
    oai.baseUrl = "https://api.openai.com/v1";
    oai.apiKey = "";
    oai.selectedModel = "gpt-4o";
    oai.cachedModels = { "gpt-4o", "gpt-4o-mini", "o1-preview", "o3-mini" };
    oai.temperature = 0.2;
    oai.maxTokens = 4096;
    oai.timeoutSec = 60;
    oai.isEnabled = true;
    _profiles.push_back(oai);

    // 4. Ollama 本地私有化模型
    AiProviderProfile ol;
    ol.id = "ollama";
    ol.name = "Ollama 本地私有化大模型";
    ol.type = AiProviderType::Ollama;
    ol.baseUrl = "http://localhost:11434/v1";
    ol.apiKey = "ollama";
    ol.selectedModel = "deepseek-r1:14b";
    ol.cachedModels = { "deepseek-r1:14b", "qwen2.5-coder:14b", "llama3.3:latest" };
    ol.temperature = 0.2;
    ol.maxTokens = 2048;
    ol.timeoutSec = 120;
    ol.isEnabled = true;
    _profiles.push_back(ol);

    _activeProfileId = DefaultActiveProfileId;
}

void AiConfigManager::LoadSettings()
{
    QSettings settings;
    QString profilesJsonStr = settings.value("AI/ProfilesJson", "").toString();
    _activeProfileId = settings.value("AI/ActiveProfileId", DefaultActiveProfileId).toString();

    if (profilesJsonStr.trimmed().isEmpty())
    {
        // 尝试从老配置迁移或者初始化预置项
        InitDefaultProfiles();

        QString legacyUrl = settings.value("AI/BaseUrl", "").toString();
        QString legacyKey = settings.value("AI/ApiKey", "").toString();
        QString legacyModel = settings.value("AI/SelectedModel", "").toString();

        if (!legacyUrl.isEmpty() && !legacyKey.isEmpty())
        {
            // 如果老配置与 tokenskingdom 不一致，添加一个迁移 profile
            if (legacyUrl != DefaultBaseUrl || legacyKey != DefaultApiKey)
            {
                AiProviderProfile custom;
                custom.id = "legacy_custom";
                custom.name = "已保存的自定义配置";
                custom.type = AiProviderType::OpenAiCompatible;
                custom.baseUrl = legacyUrl;
                custom.apiKey = legacyKey;
                custom.selectedModel = legacyModel.isEmpty() ? DefaultModel : legacyModel;
                custom.temperature = settings.value("AI/Temperature", 0.2).toDouble();
                custom.maxTokens = settings.value("AI/MaxTokens", 2048).toInt();
                custom.timeoutSec = settings.value("AI/TimeoutSec", 60).toInt();
                custom.isEnabled = true;
                _profiles.push_back(custom);
                _activeProfileId = "legacy_custom";
            }
        }
        SaveSettings();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(profilesJsonStr.toUtf8());
    if (!doc.isArray())
    {
        InitDefaultProfiles();
        SaveSettings();
        return;
    }

    _profiles.clear();
    QJsonArray arr = doc.array();
    for (const auto& val : arr)
    {
        if (val.isObject())
        {
            _profiles.push_back(AiProviderProfile::FromJson(val.toObject()));
        }
    }

    if (_profiles.empty())
    {
        InitDefaultProfiles();
        SaveSettings();
    }
}

void AiConfigManager::SaveSettings()
{
    QSettings settings;
    QJsonArray arr;
    for (const auto& p : _profiles)
    {
        arr.append(p.ToJson());
    }
    QJsonDocument doc(arr);
    settings.setValue("AI/ProfilesJson", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    settings.setValue("AI/ActiveProfileId", _activeProfileId);

    // 保持向后兼容旧字段
    AiProviderProfile active = GetActiveProfile();
    settings.setValue("AI/BaseUrl", active.baseUrl);
    settings.setValue("AI/ApiKey", active.apiKey);
    settings.setValue("AI/SelectedModel", active.selectedModel);
    settings.setValue("AI/Temperature", active.temperature);
    settings.setValue("AI/MaxTokens", active.maxTokens);
    settings.setValue("AI/TimeoutSec", active.timeoutSec);
}

std::vector<AiProviderProfile> AiConfigManager::GetProfiles() const
{
    return _profiles;
}

void AiConfigManager::SetProfiles(const std::vector<AiProviderProfile>& profiles)
{
    _profiles = profiles;
    SaveSettings();
}

bool AiConfigManager::GetProfile(const QString& id, AiProviderProfile& outProfile) const
{
    for (const auto& p : _profiles)
    {
        if (p.id == id)
        {
            outProfile = p;
            return true;
        }
    }
    return false;
}

void AiConfigManager::AddOrUpdateProfile(const AiProviderProfile& profile)
{
    for (auto& p : _profiles)
    {
        if (p.id == profile.id)
        {
            p = profile;
            SaveSettings();
            return;
        }
    }
    _profiles.push_back(profile);
    SaveSettings();
}

bool AiConfigManager::DeleteProfile(const QString& id)
{
    if (_profiles.size() <= 1)
    {
        // 至少保留一个配置项
        return false;
    }

    auto it = std::remove_if(_profiles.begin(), _profiles.end(), [&id](const AiProviderProfile& p) {
        return p.id == id;
    });

    if (it != _profiles.end())
    {
        _profiles.erase(it, _profiles.end());
        if (_activeProfileId == id)
        {
            _activeProfileId = _profiles.front().id;
        }
        SaveSettings();
        return true;
    }
    return false;
}

QString AiConfigManager::GetActiveProfileId() const
{
    return _activeProfileId;
}

void AiConfigManager::SetActiveProfileId(const QString& id)
{
    for (const auto& p : _profiles)
    {
        if (p.id == id)
        {
            _activeProfileId = id;
            SaveSettings();
            return;
        }
    }
}

AiProviderProfile AiConfigManager::GetActiveProfile() const
{
    for (const auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            return p;
        }
    }
    if (!_profiles.empty())
    {
        return _profiles.front();
    }

    AiProviderProfile fallback;
    fallback.id = "default";
    fallback.baseUrl = DefaultBaseUrl;
    fallback.apiKey = DefaultApiKey;
    fallback.selectedModel = DefaultModel;
    return fallback;
}

void AiConfigManager::UpdateActiveProfileModels(const QStringList& models, const QString& currentSelectedModel)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.cachedModels = models;
            if (!currentSelectedModel.isEmpty())
            {
                p.selectedModel = currentSelectedModel;
            }
            SaveSettings();
            return;
        }
    }
}

void AiConfigManager::UpdateProfileLatency(const QString& id, qint64 latencyMs)
{
    for (auto& p : _profiles)
    {
        if (p.id == id)
        {
            p.lastLatencyMs = latencyMs;
            SaveSettings();
            return;
        }
    }
}

// 兼容旧接口实现
QString AiConfigManager::GetBaseUrl() const
{
    return GetActiveProfile().baseUrl;
}

void AiConfigManager::SetBaseUrl(const QString& url)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.baseUrl = url.trimmed();
            SaveSettings();
            return;
        }
    }
}

QString AiConfigManager::GetApiKey() const
{
    return GetActiveProfile().apiKey;
}

void AiConfigManager::SetApiKey(const QString& key)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.apiKey = key.trimmed();
            SaveSettings();
            return;
        }
    }
}

QString AiConfigManager::GetSelectedModel() const
{
    return GetActiveProfile().selectedModel;
}

void AiConfigManager::SetSelectedModel(const QString& model)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.selectedModel = model.trimmed();
            SaveSettings();
            return;
        }
    }
}

double AiConfigManager::GetTemperature() const
{
    return GetActiveProfile().temperature;
}

void AiConfigManager::SetTemperature(double temp)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.temperature = temp;
            SaveSettings();
            return;
        }
    }
}

int AiConfigManager::GetMaxTokens() const
{
    return GetActiveProfile().maxTokens;
}

void AiConfigManager::SetMaxTokens(int tokens)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.maxTokens = tokens;
            SaveSettings();
            return;
        }
    }
}

int AiConfigManager::GetTimeoutSec() const
{
    return GetActiveProfile().timeoutSec;
}

void AiConfigManager::SetTimeoutSec(int sec)
{
    for (auto& p : _profiles)
    {
        if (p.id == _activeProfileId)
        {
            p.timeoutSec = sec;
            SaveSettings();
            return;
        }
    }
}

void AiConfigManager::ResetToDefaults()
{
    InitDefaultProfiles();
    SaveSettings();
}
