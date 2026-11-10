#pragma once

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

/**
 * @brief AI 服务商协议类型
 */
enum class AiProviderType
{
    OpenAiCompatible = 0, // OpenAI 兼容协议 (支持各类中转、聚合转发如 TokensKingdom, OneAPI, NewAPI 等)
    DeepSeek,             // DeepSeek 官方 API
    OpenAi,               // OpenAI 官方 API
    Ollama,               // 本地 Ollama 私有化大模型服务
    Custom                // 自定义私有模型网关
};

/**
 * @brief 类似 CC Switch 的服务商配置项 (Provider Profile)
 */
struct AiProviderProfile
{
    QString id;                 ///< 唯一标识符，例如 "tokenskingdom", "deepseek_official", "ollama_local"
    QString name;               ///< 显示名称，例如 "TokensKingdom 聚合服务"
    AiProviderType type = AiProviderType::OpenAiCompatible; ///< 协议类型
    QString baseUrl;            ///< 接口基础地址，例如 "https://api.tokenskingdom.com/v1"
    QString apiKey;             ///< 接口认证密钥
    QString selectedModel;      ///< 默认选中的模型名称，例如 "gpt-6.1-sol"
    QStringList cachedModels;   ///< 该服务商下拉缓存的可用模型列表
    double temperature = 0.2;   ///< 生成多样性 (0.0 - 1.0)
    int maxTokens = 2048;       ///< 最大生成 Token 数量
    int timeoutSec = 60;        ///< 网络超时时间 (秒)
    bool isEnabled = true;      ///< 是否启用此服务商
    qint64 lastLatencyMs = -1;  ///< 最近一次连通性测速延迟 (毫秒, -1 表示未测试或离线)

    QJsonObject ToJson() const;
    static AiProviderProfile FromJson(const QJsonObject& json);
    static QString ProviderTypeToString(AiProviderType type);
    static AiProviderType StringToProviderType(const QString& str);
};

/**
 * @brief AI 助手多服务商与全局配置持久化管理类 (单例模式)
 *        支持类似 CC Switch 的多 Provider 切换、测速与模型管理
 */
class AiConfigManager
{
public:
    static AiConfigManager& Instance()
    {
        static AiConfigManager instance;
        return instance;
    }

    // ==========================================
    // 多服务商 Profile 管理接口
    // ==========================================

    /**
     * @brief 获取所有配置的服务商列表
     */
    std::vector<AiProviderProfile> GetProfiles() const;

    /**
     * @brief 保存全量服务商列表并持久化到本地配置
     */
    void SetProfiles(const std::vector<AiProviderProfile>& profiles);

    /**
     * @brief 获取指定 ID 的服务商配置
     */
    bool GetProfile(const QString& id, AiProviderProfile& outProfile) const;

    /**
     * @brief 新增或更新单个服务商配置
     */
    void AddOrUpdateProfile(const AiProviderProfile& profile);

    /**
     * @brief 删除指定服务商配置
     */
    bool DeleteProfile(const QString& id);

    /**
     * @brief 获取当前激活的服务商 ID
     */
    QString GetActiveProfileId() const;

    /**
     * @brief 设置当前激活的服务商 ID
     */
    void SetActiveProfileId(const QString& id);

    /**
     * @brief 获取当前激活的服务商配置实体
     */
    AiProviderProfile GetActiveProfile() const;

    /**
     * @brief 更新当前激活服务商的模型缓存
     */
    void UpdateActiveProfileModels(const QStringList& models, const QString& currentSelectedModel = QString());

    /**
     * @brief 更新指定服务商的测速延迟
     */
    void UpdateProfileLatency(const QString& id, qint64 latencyMs);

    // ==========================================
    // 兼容旧接口 (自动映射至当前激活服务商)
    // ==========================================

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

    /**
     * @brief 重置为系统默认出厂配置
     */
    void ResetToDefaults();

private:
    AiConfigManager();
    ~AiConfigManager() = default;
    AiConfigManager(const AiConfigManager&) = delete;
    AiConfigManager& operator=(const AiConfigManager&) = delete;

    std::vector<AiProviderProfile> _profiles;
    QString _activeProfileId;

    void LoadSettings();
    void SaveSettings();
    void InitDefaultProfiles();
};
