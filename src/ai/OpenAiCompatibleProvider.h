#pragma once

#include "IAiProvider.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief OpenAI 兼容协议 AI 服务提供实现类 (Qt Network 异步通信)
 */
class OpenAiCompatibleProvider : public QObject, public IAiProvider
{
    Q_OBJECT

public:
    explicit OpenAiCompatibleProvider(
        const QString& baseUrl,
        const QString& apiKey,
        QObject* parent = nullptr);
    ~OpenAiCompatibleProvider() override = default;

    void SetBaseUrl(const QString& baseUrl) { _baseUrl = baseUrl; }
    QString GetBaseUrl() const { return _baseUrl; }

    void SetApiKey(const QString& apiKey) { _apiKey = apiKey; }
    QString GetApiKey() const { return _apiKey; }

    void SetTimeoutMs(int timeoutMs) { _timeoutMs = timeoutMs; }
    int GetTimeoutMs() const { return _timeoutMs; }

    void FetchModelsAsync(
        std::function<void(bool success, const QStringList& models, const QString& error)> callback) override;

    void ChatCompletionAsync(
        const QJsonArray& messages,
        const QString& model,
        double temperature,
        int maxTokens,
        std::function<void(bool success, const QString& reply, const QString& error)> callback) override;

private:
    QString _baseUrl;
    QString _apiKey;
    int _timeoutMs = 60000; // 默认 60 秒超时
    QNetworkAccessManager _netManager;

    QString NormalizeUrl(const QString& endpoint) const;
};
